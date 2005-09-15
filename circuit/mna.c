/*	$Csoft: mna.c,v 1.3 2005/09/13 03:51:41 vedge Exp $	*/

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

#include <engine/engine.h>
#include <engine/widget/gui.h>
#include <engine/widget/matview.h>

#include <circuit/circuit.h>

#include <component/component.h>
#include <component/vsource.h>

#include <mat/mat.h>

#include "sim.h"

const struct sim_ops mna_ops;

struct mna {
	struct sim sim;
	Uint32 Telapsed;		/* Simulated time elapsed (ns) */
	int speed;			/* Simulation speed (updates/sec) */
	struct timeout update_to;	/* Timer for simulation updates */
	
	mat_t *A;			/* Block matrix of [G,B;C,D] */
	mat_t *A_LU;			/* LU-decomposed form of A */
	mat_t *G;			/* Passive element interconnects */
	mat_t *B;			/* Voltage sources */
	mat_t *C;			/* Transpose of [B] */
	mat_t *D;			/* Dependent sources */

	vec_t *z;			/* Block partitioned vector of [i;e] */
	vec_t *i;			/* Sum of independent current sources */
	vec_t *e;			/* Independent voltage sources */

	vec_t *x;			/* Block partitioned vector of [v;j] */
	vec_t *v;			/* Unknown voltages */
	vec_t *j;			/* Unknown currents thru vsources */
};

static int
mna_solve(struct mna *mna, struct circuit *ckt)
{
	struct component *com;
	u_int i, n, m;
	veci_t *iv;
	double d;

	mat_set(mna->G, 0.0);
	mat_set(mna->B, 0.0);
	mat_set(mna->C, 0.0);
	mat_set(mna->D, 0.0);

	for (n = 1; n <= ckt->n; n++) {
		struct cktnode *node = ckt->nodes[n];
		struct cktbranch *br;
		struct component *com;
		struct dipole *dip;

		mna->G->mat[n][n] = 0.0;

		if (n > 0) {
			/* Load voltage sources into the B and C matrices. */
			for (m = 1; m <= ckt->m; m++) {
				int sign;

				if (cktnode_vsource(ckt, n, m, &sign)) {
					mna->B->mat[n][m] = sign;
					mna->C->mat[m][n] = sign;
				}
			}
		}

		/* Load passive components into the G matrix. */
		TAILQ_FOREACH(br, &node->branches, branches) {
			if ((com = br->pin->com) == NULL)
				continue;

			for (i = 0; i < com->ndipoles; i++) {
				struct dipole *dip = &com->dipoles[i];
				int grounded;
				double r;
			
				if (dip->p1 != br->pin &&
				    dip->p2 != br->pin) {
					continue;
				}
				if ((dip->p1 == br->pin && dip->p2->node == 0)||
				    (dip->p2 == br->pin && dip->p1->node == 0)){
					grounded = 1;
				} else {
					grounded = 0;
				}

				if (com->ops->resistance == NULL) {
					continue;
				}
				r = com->ops->resistance(com, dip->p1, dip->p2);

				/*
				 * The main diagonal contains the sum of the
				 * conductances connected to node n.
				 */
				if (isinf(1.0/r)) {
					mna->G->mat[n][n] += 10000000.0;
				} else {
					mna->G->mat[n][n] += 1.0/r;
				}

				/*
				 * If the dipole is ungrounded, add its negative
				 * conductance to elements G(n1,n2) and
				 * G(n2,n1).
				 */
				if (!grounded) {
					u_int n1 = dip->p1->node;
					u_int n2 = dip->p2->node;

					if (isinf(1.0/r)) {
						mna->G->mat[n1][n2]+=10000000.0;
						mna->G->mat[n2][n1]+=10000000.0;
					} else {
						mna->G->mat[n1][n2] += -(1.0/r);
						mna->G->mat[n2][n1] += -(1.0/r);
					}
				}
			}
		}
	}

	/* Build the vector of current and voltage sources. */
	for (n = 1; n <= ckt->n; n++) {
		mna->i->mat[n][1] = 0.0;	/* TODO */
	}
	for (m = 1; m <= ckt->m; m++) {
		mna->e->mat[m][1] = VSOURCE(ckt->vsrcs[m-1])->voltage;
	}
	mat_compose21(mna->z, mna->i, mna->e);

	/*
	 * Generate the partitioned matrix A with blocks GBCD, compute the
	 * LU decomposition and solve the system by backsubstitution.
	 */
	mat_compose22(mna->A, mna->G, mna->B, mna->C, mna->D);
	iv = veci_new(mna->z->m);
	if ((mat_lu_decompose(mna->A, mna->A_LU, iv, &d)) == NULL) {
		error_set("A(lu): %s", error_get());
		goto fail;
	}
	vec_copy(mna->z, mna->x);
	mat_lu_backsubst(mna->A_LU, iv, mna->x);

	veci_free(iv);
	return (0);
fail:
	veci_free(iv);
	return (-1);
}

static Uint32
mna_tick(void *obj, Uint32 ival, void *arg)
{
	struct circuit *ckt = obj;
	struct sim *sim = arg;
	struct mna *mna = arg;
	struct component *com;

	/* Effect the independent voltage sources. XXX */
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!OBJECT_SUBCLASS(com, "component.vsource") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		if (com->ops->tick != NULL)
			com->ops->tick(com);
	}
	/* Update model states. XXX */
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!OBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		if (com->ops->tick != NULL)
			com->ops->tick(com);
	}

	if (mna_solve(mna, ckt) == -1) {
		text_msg(MSG_ERROR, "%s", error_get());
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
	timeout_set(&mna->update_to, mna_tick, mna, TIMEOUT_LOADABLE);

	mna->A = mat_new(0, 0);
	mna->A_LU = mat_new(0, 0);
	mna->G = mat_new(0, 0);
	mna->B = mat_new(0, 0);
	mna->C = mat_new(0, 0);
	mna->D = mat_new(0, 0);

	mna->z = vec_new(0);
	mna->i = vec_new(0);
	mna->e = vec_new(0);
	
	mna->x = vec_new(0);
	mna->v = vec_new(0);
	mna->j = vec_new(0);
}

static void
mna_start(struct mna *mna)
{
	struct circuit *ckt = SIM(mna)->ckt;

	lock_timeout(ckt);
	if (timeout_scheduled(ckt, &mna->update_to)) {
		timeout_del(ckt, &mna->update_to);
	}
	timeout_add(ckt, &mna->update_to, 1000/mna->speed);
	unlock_timeout(ckt);

	mna->Telapsed = 0;
}

static void
mna_stop(struct mna *mna)
{
	struct circuit *ckt = SIM(mna)->ckt;

	lock_timeout(ckt);
	if (timeout_scheduled(ckt, &mna->update_to)) {
		timeout_del(ckt, &mna->update_to);
	}
	unlock_timeout(ckt);
}

static void
mna_destroy(void *p)
{
	struct mna *mna = p;
	
	mna_stop(mna);

	mat_free(mna->A);
	mat_free(mna->A_LU);
	mat_free(mna->G);
	mat_free(mna->B);
	mat_free(mna->C);
	mat_free(mna->D);

	vec_free(mna->z);
	vec_free(mna->i);
	vec_free(mna->e);

	vec_free(mna->x);
	vec_free(mna->v);
	vec_free(mna->j);
}

static void
mna_cktmod(void *p, struct circuit *ckt)
{
	struct mna *mna = p;

	mat_resize(mna->A, ckt->n+ckt->m, ckt->n+ckt->m);
	mat_resize(mna->A_LU, ckt->n+ckt->m, ckt->n+ckt->m);
	mat_resize(mna->G, ckt->n, ckt->n);
	mat_resize(mna->B, ckt->n, ckt->m);
	mat_resize(mna->C, ckt->m, ckt->n);
	mat_resize(mna->D, ckt->m, ckt->m);

	mat_resize(mna->i, ckt->n, 1);
	mat_resize(mna->e, ckt->m, 1);
	mat_resize(mna->z, ckt->n+ckt->m, 1);
	
	mat_resize(mna->v, ckt->n, 1);
	mat_resize(mna->j, ckt->m, 1);
	mat_resize(mna->x, ckt->n+ckt->m, 1);
	
	mat_set(mna->A, 0);
	mat_set(mna->A_LU, 0);
	mat_set(mna->G, 0);
	mat_set(mna->B, 0);
	mat_set(mna->C, 0);
	mat_set(mna->D, 0);

	vec_set(mna->z, 0);
	vec_set(mna->i, 0);
	
	vec_set(mna->x, 0);
	vec_set(mna->v, 0);
	vec_set(mna->j, 0);
}

static void
cont_run(int argc, union evarg *argv)
{
	struct button *bu = argv[0].p;
	struct mna *mna = argv[1].p;
	struct sim *sim = argv[1].p;
	int state = argv[2].i;

	if (state) {
		mna_cktmod(mna, sim->ckt);
		mna_start(mna);
		sim->running = 1;
		text_tmsg(MSG_INFO, 500, _("Simulation started."));
	} else {
		mna_stop(mna);
		sim->running = 0;
		text_tmsg(MSG_INFO, 500, _("Simulation stopped."));
	}
}

static struct window *
mna_edit(void *p, struct circuit *ckt)
{
	struct mna *mna = p;
	struct window *win;
	struct spinbutton *sbu;
	struct fspinbutton *fsu;
	struct tlist *tl;
	struct notebook *nb;
	struct notebook_tab *ntab;
	struct button *bu;
	struct matview *mv;

	win = window_new(0, NULL);

	nb = notebook_new(win, NOTEBOOK_WFILL|NOTEBOOK_HFILL);
	ntab = notebook_add_tab(nb, _("Continuous mode"), BOX_VERT);
	{
		bu = button_new(ntab, _("Run simulation"));
		button_set_sticky(bu, 1);
		WIDGET(bu)->flags |= WIDGET_WFILL;
		event_new(bu, "button-pushed", cont_run, "%p", mna);
	
		separator_new(ntab, SEPARATOR_HORIZ);

		sbu = spinbutton_new(ntab, _("Speed (updates/sec): "));
		widget_bind(sbu, "value", WIDGET_UINT32, &mna->speed);
		spinbutton_set_range(sbu, 1, 1000);
		
		label_new(ntab, LABEL_POLLED, _("Elapsed time: %[u32]ns"),
		    &mna->Telapsed);
	}
	
	ntab = notebook_add_tab(nb, "[A]", BOX_VERT);
	mv = matview_new(ntab, mna->A, 0);
	matview_prescale(mv, "-0.000", 10, 5);
	matview_set_numfmt(mv, "%.03f");
	
	ntab = notebook_add_tab(nb, "[A]-LU", BOX_VERT);
	mv = matview_new(ntab, mna->A_LU, 0);
	matview_prescale(mv, "-0.000", 10, 5);
	matview_set_numfmt(mv, "%.03f");

	ntab = notebook_add_tab(nb, "[G]", BOX_VERT);
	mv = matview_new(ntab, mna->G, 0);
	matview_prescale(mv, "-0.000", 10, 5);
	matview_set_numfmt(mv, "%.03f");
	
	ntab = notebook_add_tab(nb, "[B]", BOX_VERT);
	mv = matview_new(ntab, mna->B, 0);
	matview_prescale(mv, "-0.000", 10, 5);
	matview_set_numfmt(mv, "%.03f");
	
	ntab = notebook_add_tab(nb, "[C]", BOX_VERT);
	mv = matview_new(ntab, mna->C, 0);
	matview_prescale(mv, "-0.000", 10, 5);
	matview_set_numfmt(mv, "%.03f");
	
	ntab = notebook_add_tab(nb, "[D]", BOX_VERT);
	mv = matview_new(ntab, mna->D, 0);
	matview_prescale(mv, "-0.000", 10, 5);
	matview_set_numfmt(mv, "%.03f");
	
	ntab = notebook_add_tab(nb, "[z]", BOX_VERT);
	mv = matview_new(ntab, mna->z, 0);
	matview_prescale(mv, "-0000.0000", 10, 5);
	matview_set_numfmt(mv, "%.03f");
	
	ntab = notebook_add_tab(nb, "[x]", BOX_VERT);
	mv = matview_new(ntab, mna->x, 0);
	matview_prescale(mv, "-0000.0000", 10, 5);
	matview_set_numfmt(mv, "%.03f");
	
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
