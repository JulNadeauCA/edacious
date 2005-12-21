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

const ES_SimOps esMnaOps;

typedef struct es_mna {
	ES_Sim sim;
	Uint32 Telapsed;	/* Simulated time elapsed (ns) */
	int speed;		/* Simulation speed (updates/sec) */
	AG_Timeout update_to;	/* Timer for simulation updates */

	SC_Matrix *A;		/* Block matrix [G,B; C,D] */
	SC_Matrix *G;		/* Reduced conductance matrix */
	SC_Matrix *B;		/* Voltage source matrix */
	SC_Matrix *C;		/* Transpose of B */
	SC_Matrix *D;		/* Dependent source matrix */
	SC_Matrix *LU;		/* LU factorizations of A */

	SC_Vector *z;		/* Right-hand side vector (i,e) */
	SC_Vector *i;		/* Independent current sources */
	SC_Vector *e;		/* Independent voltage sources */

	SC_Vector *x;		/* Vector of unknowns (v,j) */
	SC_Vector *v;		/* Voltages */
	SC_Vector *j;		/* Currents through independent vsources */
	
	SC_Ivector *piv;	/* Pivot information from last factorization */
} ES_MNA;

/* Formulate the KCL/branch equations and compute A=LU. */
static int
ES_MnaFactorize(ES_MNA *mna, ES_Circuit *ckt)
{
	ES_Component *com;
	u_int i, n, m;
	SC_Real d;

	SC_MatrixSetZero(mna->G);
	SC_MatrixSetZero(mna->B);
	SC_MatrixSetZero(mna->C);
	SC_MatrixSetZero(mna->D);
	SC_MatrixSetZero(mna->i);
	SC_MatrixSetZero(mna->e);

	/* Formulate the general equations. */
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

	/* Compose the right-hand side vector (i, e). */
	SC_MatrixCompose21(mna->z, mna->i, mna->e);

	/* Compose the coefficient matrix A from [G,B;C,D] and compute LU. */
	SC_MatrixCompose22(mna->A, mna->G, mna->B, mna->C, mna->D);
	return (((SC_FactorizeLU(mna->A, mna->LU, mna->piv, &d)) != NULL) ?
	        0 : -1);
}

static int
ES_MnaSolve(ES_MNA *mna, ES_Circuit *ckt)
{
	SC_VectorCopy(mna->z, mna->x);
	SC_BacksubstLU(mna->LU, mna->piv, mna->x);
	return (0);
}

static void
ES_MnaDumpX(ES_Circuit *ckt, ES_MNA *mna)
{
	//ES_CircuitLog()
}

static Uint32
ES_MnaStep(void *obj, Uint32 ival, void *arg)
{
	ES_Circuit *ckt = obj;
	ES_MNA *mna = arg;
	ES_Component *com;
	int i;

	if (mna->Telapsed == 0) {
		if (ES_MnaFactorize(mna, ckt) == -1) {
			AG_TextMsg(AG_MSG_ERROR, "%s", AG_GetError());
			return (0);
		}
		ES_MnaSolve(mna, ckt);
		ES_MnaDumpX(ckt, mna);
	}

	AGOBJECT_FOREACH_CHILD(com, ckt, es_component) {
		if (!AGOBJECT_SUBCLASS(com, "component")) {
			continue;
		}
		if (com->intStep != NULL)
			com->intStep(com, mna->Telapsed);
		if (com->intUpdate != NULL)
			com->intUpdate(com);
	}
	if (ES_MnaFactorize(mna, ckt) == -1) {
		AG_TextMsg(AG_MSG_ERROR, "%s", AG_GetError());
		return (0);
	}
	ES_MnaSolve(mna, ckt);

	mna->Telapsed++;
	return (ival);
}

static void
ES_MnaInit(void *p)
{
	ES_MNA *mna = p;

	ES_SimInit(mna, &esMnaOps);
	mna->Telapsed = 0;
	mna->speed = 10;
	AG_SetTimeout(&mna->update_to, ES_MnaStep, mna, AG_TIMEOUT_LOADABLE);

	mna->A = SC_MatrixNew(0, 0);
	mna->G = SC_MatrixNew(0, 0);
	mna->B = SC_MatrixNew(0, 0);
	mna->C = SC_MatrixNew(0, 0);
	mna->D = SC_MatrixNew(0, 0);
	mna->LU = SC_MatrixNew(0, 0);

	mna->z = SC_VectorNew(0);
	mna->i = SC_VectorNew(0);
	mna->e = SC_VectorNew(0);
	
	mna->x = SC_VectorNew(0);
	mna->v = SC_VectorNew(0);
	mna->j = SC_VectorNew(0);

	mna->piv = SC_IvectorNew(0);
}

static void
ES_MnaStartContinuous(ES_MNA *mna)
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
ES_MnaStopContinuous(ES_MNA *mna)
{
	ES_Circuit *ckt = SIM(mna)->ckt;

	AG_LockTimeouts(ckt);
	if (AG_TimeoutIsScheduled(ckt, &mna->update_to)) {
		AG_DelTimeout(ckt, &mna->update_to);
	}
	AG_UnlockTimeouts(ckt);
}

static void
ES_MnaDestroy(void *p)
{
	ES_MNA *mna = p;
	
	ES_MnaStopContinuous(mna);

	SC_MatrixFree(mna->A);
	SC_MatrixFree(mna->G);
	SC_MatrixFree(mna->B);
	SC_MatrixFree(mna->C);
	SC_MatrixFree(mna->D);
	SC_MatrixFree(mna->LU);
	SC_IvectorFree(mna->piv);
	SC_VectorFree(mna->z);
	SC_VectorFree(mna->i);
	SC_VectorFree(mna->e);
	SC_VectorFree(mna->x);
	SC_VectorFree(mna->v);
	SC_VectorFree(mna->j);
}

static void
ES_MnaResize(void *p, ES_Circuit *ckt)
{
	ES_MNA *mna = p;
	Uint n = ckt->n;
	Uint m = ckt->m;

	SC_MatrixResize(mna->A, n+m, n+m);
	SC_MatrixResize(mna->G, n, n);
	SC_MatrixResize(mna->B, n, m);
	SC_MatrixResize(mna->C, m, n);
	SC_MatrixResize(mna->D, m, m);
	SC_MatrixResize(mna->LU, n+m, n+m);
	SC_IvectorResize(mna->piv, n+m);
	SC_MatrixResize(mna->z, n+m, 1);
	SC_MatrixResize(mna->i, n, 1);
	SC_MatrixResize(mna->e, m, 1);
	SC_MatrixResize(mna->x, n+m, 1);
	SC_MatrixResize(mna->v, n, 1);
	SC_MatrixResize(mna->j, m, 1);

	SC_MatrixSetZero(mna->A);
	SC_MatrixSetZero(mna->G);
	SC_MatrixSetZero(mna->B);
	SC_MatrixSetZero(mna->C);
	SC_MatrixSetZero(mna->D);
	SC_MatrixSetZero(mna->LU);
	SC_IvectorSetZero(mna->piv);
	SC_VectorSetZero(mna->z);
	SC_VectorSetZero(mna->i);
	SC_VectorSetZero(mna->e);
	SC_VectorSetZero(mna->x);
	SC_VectorSetZero(mna->v);
	SC_VectorSetZero(mna->j);
}

static void
ES_MnaContinuous(AG_Event *event)
{
	AG_Button *bu = AG_SELF();
	ES_Sim *sim = AG_PTR(1);
	int state = AG_INT(2);
	ES_MNA *mna = (ES_MNA *)sim;

	if (state) {
		ES_MnaResize(sim, sim->ckt);
		ES_MnaStartContinuous(mna);
		sim->running = 1;
		ES_SimLog(mna, _("Simulation started."));
	} else {
		ES_MnaStopContinuous(mna);
		sim->running = 0;
		ES_SimLog(mna, _("Simulation stopped at %uns."), mna->Telapsed);
	}
}

static AG_Window *
ES_MnaEdit(void *p, ES_Circuit *ckt)
{
	ES_MNA *mna = p;
	AG_Window *win;
	AG_Spinbutton *sbu;
	AG_FSpinbutton *fsu;
	AG_Tlist *tl;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;
	AG_Matview *mv;

	win = AG_WindowNew(AG_WINDOW_NOCLOSE);

	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);
	ntab = AG_NotebookAddTab(nb, _("Continuous mode"), AG_BOX_VERT);
	{
		AG_ButtonAct(ntab, AG_BUTTON_HFILL|AG_BUTTON_STICKY,
		    _("Run simulation"),
		    ES_MnaContinuous, "%p", mna);
	
		AG_SeparatorNew(ntab, AG_SEPARATOR_HORIZ);

		sbu = AG_SpinbuttonNew(ntab, 0, _("Speed (updates/sec): "));
		AG_WidgetBind(sbu, "value", AG_WIDGET_UINT32, &mna->speed);
		AG_SpinbuttonSetRange(sbu, 1, 1000);
		
		AG_LabelNew(ntab, AG_LABEL_POLLED, _("Elapsed time: %[u32]ns"),
		    &mna->Telapsed);

		SIM(mna)->log = AG_ConsoleNew(ntab, AG_CONSOLE_EXPAND);
	}
	
	ntab = AG_NotebookAddTab(nb, "[A]", AG_BOX_VERT);
	mv = AG_MatviewNew(ntab, mna->A, 0);
	AG_MatviewPrescale(mv, "-0.000", 10, 5);
	AG_MatviewSetNumericalFmt(mv, "%.03f");
	
	ntab = AG_NotebookAddTab(nb, "[LU]", AG_BOX_VERT);
	mv = AG_MatviewNew(ntab, mna->LU, 0);
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

static SC_Real
ES_MnaNodeVoltage(void *p, int j)
{
	ES_MNA *mna = p;

	if (j > 0 && j <= SIM(mna)->ckt->n) {
		return (mna->x->mat[j][1]);
	} else {
		return (0.0);
	}
}

static SC_Real
ES_MnaBranchCurrent(void *p, int k)
{
	ES_MNA *mna = p;
	
	if (k > 0 && k <= SIM(mna)->ckt->n + SIM(mna)->ckt->m) {
		return (mna->x->mat[SIM(mna)->ckt->n + k][1]);
	} else {
		return (0.0);
	}
}

const ES_SimOps esMnaOps = {
	N_("Modified Nodal Analysis"),
	EDA_NODE_ICON,
	sizeof(ES_MNA),
	ES_MnaInit,
	ES_MnaDestroy,
	ES_MnaResize,
	ES_MnaNodeVoltage,
	ES_MnaBranchCurrent,
	ES_MnaEdit
};
