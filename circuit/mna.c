/*	$Csoft: mna.c,v 1.6 2005/09/29 00:22:34 vedge Exp $	*/

/*
 * Copyright (c) 2005 CubeSoft Communications, Inc.
 * <http://www.winds-triton.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <agar/core.h>
#include <agar/sc.h>
#include <agar/gui.h>
#include <agar/sc/sc_matview.h>

#include "eda.h"
#include "sim.h"

const struct sim_ops mna_ops;

struct mna {
	struct sim sim;
	Uint32 Telapsed;		/* Simulated time elapsed (ns) */
	int speed;			/* Simulation speed (updates/sec) */
	AG_Timeout update_to;		/* Timer for simulation updates */
	
	SC_Matrix *A;			/* Block matrix [G B; C D] */
	SC_Matrix *A_LU;		/* LU factorization of A */
	SC_Matrix *G;			/* Reduced conductance matrix */
	SC_Matrix *B;			/* Voltage source matrix */
	SC_Matrix *C;			/* Transpose of B */
	SC_Matrix *D;			/* Dependent source matrix */

	SC_Vector *z;			/* Block matrix (i e) */
	SC_Vector *i;			/* Sum of independent current sources */
	SC_Vector *e;			/* Independent voltage sources */

	SC_Vector *x;			/* Block matrix (v j) */
	SC_Vector *v;			/* Unknown voltages */
	SC_Vector *j;			/* Unknown currents thru vsources */
};

static int
mna_solve(struct mna *mna, ES_Circuit *ckt)
{
	ES_Component *com;
	u_int i, n, m;
	SC_Ivector *iv;
	double d;

	SC_MatrixSetZero(mna->G);
	SC_MatrixSetZero(mna->B);
	SC_MatrixSetZero(mna->C);
	SC_MatrixSetZero(mna->D);
	SC_MatrixSetZero(mna->i);
	SC_MatrixSetZero(mna->e);
	
	AGOBJECT_FOREACH_CHILD(com, ckt, es_component) {
		if (!AGOBJECT_SUBCLASS(com, "component")) {
			continue;
		}
		if (com->loadDC_G != NULL)
			com->loadDC_G(com, mna->G);
		if (com->loadDC_BCD != NULL)
			com->loadDC_BCD(com, mna->B, mna->C, mna->D);
		if (com->loadDC_RHS != NULL)
			com->loadDC_RHS(com, mna->i, mna->e);
#if 0
		if (com->loadAC != NULL)
			com->loadAC(com, mna->Y);
		if (com->loadSP != NULL)
			com->loadSP(com, mna->S, mna->N);
#endif
	}
#if 0
	for (n = 1; n <= ckt->n; n++) {
		ES_Node *node = ckt->nodes[n];
		ES_Branch *br;
		ES_Component *com;
		ES_Pair *pair;

		mna->G->mat[n][n] = 0.0;

		if (n > 0) {
			/* Load voltage sources into the B and C matrices. */
			for (m = 1; m <= ckt->m; m++) {
				int sign;

				if (ES_NodeVsource(ckt, n, m, &sign)) {
					mna->B->mat[n][m] = sign;
					mna->C->mat[m][n] = sign;
				}
			}
		}

		/* Load conductances into the G matrix. */
		TAILQ_FOREACH(br, &node->branches, branches) {
			if ((com = br->port->com) == NULL)
				continue;

			for (i = 0; i < com->npairs; i++) {
				ES_Pair *pair = &com->pairs[i];
				int grounded;
				double g;
			
				if (pair->p1 != br->port &&
				    pair->p2 != br->port) {
					continue;
				}
				if ((pair->p1==br->port && pair->p2->node==0)||
				    (pair->p2==br->port && pair->p1->node==0)){
					grounded = 1;
				} else {
					grounded = 0;
				} 
				if (com->ops->resistance == NULL) {
					continue;
				}
				g = com->ops->resistance(com, pair->p1,
				                              pair->p2);
				if (g == 0.0) {
					continue;
				}
				g = 1.0/g;

				/*
				 * The main diagonal contains the sum of the
				 * conductances connected to node n.
				 */
				mna->G->mat[n][n] += g;

				/*
				 * If the component is ungrounded, subtract its
				 * conductance from G(n1,n2) and G(n2,n1).
				 */
				if (!grounded) {
					u_int n1 = pair->p1->node;
					u_int n2 = pair->p2->node;

					mna->G->mat[n1][n2] -= g;
					mna->G->mat[n2][n1] -= g;
				}
			}
		}
	}

	/* Initialize the RHS vector (current and voltage sources). */
	for (m = 1; m <= ckt->m; m++) {
		mna->e->mat[m][1] = VSOURCE(ckt->vsrcs[m-1])->voltage;
	}
#endif

	SC_MatrixCompose21(mna->z, mna->i, mna->e);

	/* Build the block matrix A from GBCD and solve the system. */
	SC_MatrixCompose22(mna->A, mna->G, mna->B, mna->C, mna->D);
	iv = SC_IvectorNew(mna->z->m);
	if ((SC_FactorizeLU(mna->A, mna->A_LU, iv, &d)) == NULL) {
		AG_SetError("A(LU): %s", AG_GetError());
		goto fail;
	}
	SC_VectorCopy(mna->z, mna->x);
	SC_BacksubstLU(mna->A_LU, iv, mna->x);

	SC_IvectorFree(iv);
	return (0);
fail:
	SC_IvectorFree(iv);
	return (-1);
}

static Uint32
mna_tick(void *obj, Uint32 ival, void *arg)
{
	ES_Circuit *ckt = obj;
	struct sim *sim = arg;
	struct mna *mna = arg;
	ES_Component *com;

	if (mna_solve(mna, ckt) == -1) {
		AG_TextMsg(AG_MSG_ERROR, "%s", AG_GetError());
		return (0);
	}
	mna->Telapsed++;
	return (ival);
}

static void
mna_init(void *p)
{
	struct mna *mna = p;

	sim_init(mna, &mna_ops);
	mna->Telapsed = 0;
	mna->speed = 4;
	AG_SetTimeout(&mna->update_to, mna_tick, mna, AG_TIMEOUT_LOADABLE);

	mna->A = SC_MatrixNew(0, 0);
	mna->A_LU = SC_MatrixNew(0, 0);
	mna->G = SC_MatrixNew(0, 0);
	mna->B = SC_MatrixNew(0, 0);
	mna->C = SC_MatrixNew(0, 0);
	mna->D = SC_MatrixNew(0, 0);

	mna->z = SC_VectorNew(0);
	mna->i = SC_VectorNew(0);
	mna->e = SC_VectorNew(0);
	
	mna->x = SC_VectorNew(0);
	mna->v = SC_VectorNew(0);
	mna->j = SC_VectorNew(0);
}

static void
mna_start(struct mna *mna)
{
	ES_Circuit *ckt = SIM(mna)->ckt;

	AG_LockTimeouts(ckt);
	if (AG_TimeoutIsScheduled(ckt, &mna->update_to)) {
		AG_DelTimeout(ckt, &mna->update_to);
	}
	AG_AddTimeout(ckt, &mna->update_to, 1000/mna->speed);
	AG_UnlockTimeouts(ckt);

	mna->Telapsed = 0;
}

static void
mna_stop(struct mna *mna)
{
	ES_Circuit *ckt = SIM(mna)->ckt;

	AG_LockTimeouts(ckt);
	if (AG_TimeoutIsScheduled(ckt, &mna->update_to)) {
		AG_DelTimeout(ckt, &mna->update_to);
	}
	AG_UnlockTimeouts(ckt);
}

static void
mna_destroy(void *p)
{
	struct mna *mna = p;
	
	mna_stop(mna);

	SC_MatrixFree(mna->A);
	SC_MatrixFree(mna->A_LU);
	SC_MatrixFree(mna->G);
	SC_MatrixFree(mna->B);
	SC_MatrixFree(mna->C);
	SC_MatrixFree(mna->D);

	SC_VectorFree(mna->z);
	SC_VectorFree(mna->i);
	SC_VectorFree(mna->e);

	SC_VectorFree(mna->x);
	SC_VectorFree(mna->v);
	SC_VectorFree(mna->j);
}

static void
mna_cktmod(void *p, ES_Circuit *ckt)
{
	struct mna *mna = p;

	SC_MatrixResize(mna->A, ckt->n+ckt->m, ckt->n+ckt->m);
	SC_MatrixResize(mna->A_LU, ckt->n+ckt->m, ckt->n+ckt->m);
	SC_MatrixResize(mna->G, ckt->n, ckt->n);
	SC_MatrixResize(mna->B, ckt->n, ckt->m);
	SC_MatrixResize(mna->C, ckt->m, ckt->n);
	SC_MatrixResize(mna->D, ckt->m, ckt->m);

	SC_MatrixResize(mna->i, ckt->n, 1);
	SC_MatrixResize(mna->e, ckt->m, 1);
	SC_MatrixResize(mna->z, ckt->n+ckt->m, 1);
	
	SC_MatrixResize(mna->v, ckt->n, 1);
	SC_MatrixResize(mna->j, ckt->m, 1);
	SC_MatrixResize(mna->x, ckt->n+ckt->m, 1);
	
	SC_MatrixSetZero(mna->A);
	SC_MatrixSetZero(mna->A_LU);
	SC_MatrixSetZero(mna->G);
	SC_MatrixSetZero(mna->B);
	SC_MatrixSetZero(mna->C);
	SC_MatrixSetZero(mna->D);

	SC_VectorSetZero(mna->z);
	SC_VectorSetZero(mna->i);
	
	SC_VectorSetZero(mna->x);
	SC_VectorSetZero(mna->v);
	SC_VectorSetZero(mna->j);
}

static void
cont_run(AG_Event *event)
{
	AG_Button *bu = AG_SELF();
	struct sim *sim = AG_PTR(1);
	struct mna *mna = (struct mna *)sim;
	int state = AG_INT(2);

	if (state) {
		mna_cktmod(mna, sim->ckt);
		mna_start(mna);
		sim->running = 1;
		AG_TextTmsg(AG_MSG_INFO, 500, _("Simulation started."));
	} else {
		mna_stop(mna);
		sim->running = 0;
		AG_TextTmsg(AG_MSG_INFO, 500, _("Simulation stopped."));
	}
}

static AG_Window *
mna_edit(void *p, ES_Circuit *ckt)
{
	struct mna *mna = p;
	AG_Window *win;
	AG_Spinbutton *sbu;
	AG_FSpinbutton *fsu;
	AG_Tlist *tl;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;
	AG_Matview *mv;

	win = AG_WindowNew(0);

	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);
	ntab = AG_NotebookAddTab(nb, _("Continuous mode"), AG_BOX_VERT);
	{
		AG_ButtonAct(ntab, AG_BUTTON_HFILL|AG_BUTTON_STICKY,
		    _("Run simulation"),
		    cont_run, "%p", mna);
	
		AG_SeparatorNew(ntab, AG_SEPARATOR_HORIZ);

		sbu = AG_SpinbuttonNew(ntab, 0, _("Speed (updates/sec): "));
		AG_WidgetBind(sbu, "value", AG_WIDGET_UINT32, &mna->speed);
		AG_SpinbuttonSetRange(sbu, 1, 1000);
		
		AG_LabelNew(ntab, AG_LABEL_POLLED, _("Elapsed time: %[u32]ns"),
		    &mna->Telapsed);
	}
	
	ntab = AG_NotebookAddTab(nb, "[A]", AG_BOX_VERT);
	mv = AG_MatviewNew(ntab, mna->A, 0);
	AG_MatviewPrescale(mv, "-0.000", 10, 5);
	AG_MatviewSetNumericalFmt(mv, "%.03f");
	
	ntab = AG_NotebookAddTab(nb, "[A]-LU", AG_BOX_VERT);
	mv = AG_MatviewNew(ntab, mna->A_LU, 0);
	AG_MatviewPrescale(mv, "-0.000", 10, 5);
	AG_MatviewSetNumericalFmt(mv, "%.03f");

	ntab = AG_NotebookAddTab(nb, "[G]", AG_BOX_VERT);
	mv = AG_MatviewNew(ntab, mna->G, 0);
	AG_MatviewPrescale(mv, "-0.000", 10, 5);
	AG_MatviewSetNumericalFmt(mv, "%.03f");
	
	ntab = AG_NotebookAddTab(nb, "[B]", AG_BOX_VERT);
	mv = AG_MatviewNew(ntab, mna->B, 0);
	AG_MatviewPrescale(mv, "-0.000", 10, 5);
	AG_MatviewSetNumericalFmt(mv, "%.03f");
	
	ntab = AG_NotebookAddTab(nb, "[C]", AG_BOX_VERT);
	mv = AG_MatviewNew(ntab, mna->C, 0);
	AG_MatviewPrescale(mv, "-0.000", 10, 5);
	AG_MatviewSetNumericalFmt(mv, "%.03f");
	
	ntab = AG_NotebookAddTab(nb, "[D]", AG_BOX_VERT);
	mv = AG_MatviewNew(ntab, mna->D, 0);
	AG_MatviewPrescale(mv, "-0.000", 10, 5);
	AG_MatviewSetNumericalFmt(mv, "%.03f");
	
	ntab = AG_NotebookAddTab(nb, "[z]", AG_BOX_VERT);
	mv = AG_MatviewNew(ntab, mna->z, 0);
	AG_MatviewPrescale(mv, "-0000.0000", 10, 5);
	AG_MatviewSetNumericalFmt(mv, "%.03f");
	
	ntab = AG_NotebookAddTab(nb, "[x]", AG_BOX_VERT);
	mv = AG_MatviewNew(ntab, mna->x, 0);
	AG_MatviewPrescale(mv, "-0000.0000", 10, 5);
	AG_MatviewSetNumericalFmt(mv, "%.03f");
	
	return (win);
}

const struct sim_ops mna_ops = {
	N_("Modified Nodal Analysis"),
	EDA_NODE_ICON,
	sizeof(struct mna),
	mna_init,
	mna_destroy,
	mna_cktmod,
	mna_edit
};
