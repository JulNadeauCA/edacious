/*	$Csoft: mna.c,v 1.1 2005/09/10 05:48:20 vedge Exp $	*/

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
	
	mat_t *A;			/* Supermatrix of [G,B;C,D] */
	mat_t *G;			/* Passive element interconnects */
	mat_t *B;			/* Voltage sources */
	mat_t *C;			/* Transpose of [B] */
	mat_t *D;			/* Dependent sources */

	vec_t *z;			/* Supervector of [i;e] */
	vec_t *i;			/* Sum of independent current sources */
	vec_t *e;			/* Independent voltage sources */

	vec_t *x;			/* Supervector of [v;j] */
	vec_t *v;			/* Unknown voltages */
	vec_t *j;			/* Unknown currents thru vsources */
};

static void
mna_solve(struct mna *mna, struct circuit *ckt)
{
	struct component *com;
	int i, n;

#if 0
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!OBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		for (i = 0; i < com->ndipoles; i++) {
		}
	}
#endif

	/* Add grounded elements to the main [G] diagonal. */
	for (n = 1; n <= ckt->n; n++) {
		if (circuit_gnd_node(ckt, n)) {
			mna->G->mat[n][n] += 1.0;
		}
	}
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

	mna_solve(mna, ckt);
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
	mat_resize(mna->G, ckt->n, ckt->n);
	mat_resize(mna->B, ckt->m, ckt->n);
	mat_resize(mna->C, ckt->n, ckt->m);
	mat_resize(mna->D, ckt->m, ckt->m);
	
	mat_set(mna->A, 0);
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
poll_Gmat(int argc, union evarg *argv)
{
	char text[TLIST_LABEL_MAX];
	struct tlist *tl = argv[0].p;
	struct mna *mna = argv[1].p;
	struct sim *sim = argv[1].p;
	struct circuit *ckt = sim->ckt;
	unsigned int i, j;

	if (mna->G->mat == NULL)
		return;

	tlist_clear_items(tl);
	for (i = 1; i <= mna->G->m; i++) {
		text[0] = '\0';
		for (j = 1; j <= mna->G->n; j++) {
			char tmp[16];

			snprintf(tmp, sizeof(tmp), "%.0f ",
			    mna->G->mat[i][j]);
			strlcat(text, tmp, sizeof(text));
		}
		tlist_insert_item(tl, NULL, text, &mna->G->mat[i][j]);
	}
	tlist_restore_selections(tl);
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
	ntab = notebook_add_tab(nb, "[G]", BOX_VERT);
	{
		tl = tlist_new(ntab, TLIST_POLL);
		event_new(tl, "tlist-poll", poll_Gmat, "%p", mna);
	}
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
