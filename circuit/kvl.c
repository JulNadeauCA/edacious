/*	$Csoft: kvl.c,v 1.9 2005/09/29 00:22:34 vedge Exp $	*/

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

const struct sim_ops kvl_ops;

struct kvl {
	struct sim sim;
	Uint32 Telapsed;		/* Simulated time elapsed (ns) */
	int speed;			/* Simulation speed (updates/sec) */
	AG_Timeout update_to;	/* Timer for simulation updates */
	
	mat_t *Rmat;			/* Resistance matrix */
	vec_t *Vvec;			/* Independent voltages */
	vec_t *Ivec;			/* Vector of unknown currents */
};

static void compose_Rmat(struct kvl *, struct circuit *);
static int solve_Ivec(struct kvl *, struct circuit *);

static Uint32
kvl_tick(void *obj, Uint32 ival, void *arg)
{
	struct circuit *ckt = obj;
	struct sim *sim = arg;
	struct kvl *kvl = arg;
	struct component *com;

	/* Effect the independent voltage sources. */
	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!AGOBJECT_SUBCLASS(com, "component.vsource") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		if (com->ops->tick != NULL)
			com->ops->tick(com);
	}

	/* Update model states. */
	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!AGOBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		if (com->ops->tick != NULL)
			com->ops->tick(com);
	}

	/* Solve the loop currents. */
	compose_Rmat(kvl, ckt);
	if (solve_Ivec(kvl, ckt) == -1)
		fprintf(stderr, "%s: %s\n", AGOBJECT(ckt)->name, AG_GetError());

	kvl->Telapsed++;
	return (ival);
}

static void
kvl_init(void *p)
{
	struct kvl *kvl = p;

	sim_init(kvl, &kvl_ops);
	kvl->Telapsed = 0;
	kvl->speed = 4;
	kvl->Rmat = mat_new(0, 0);
	kvl->Vvec = vec_new(0);
	kvl->Ivec = vec_new(0);
	AG_SetTimeout(&kvl->update_to, kvl_tick, kvl, AG_TIMEOUT_LOADABLE);
}

static void
kvl_start(struct kvl *kvl)
{
	struct circuit *ckt = SIM(kvl)->ckt;

	AG_LockTimeouts(ckt);
	if (AG_TimeoutIsScheduled(ckt, &kvl->update_to)) {
		AG_DelTimeout(ckt, &kvl->update_to);
	}
	AG_AddTimeout(ckt, &kvl->update_to, 1000/kvl->speed);
	AG_UnlockTimeouts(ckt);

	kvl->Telapsed = 0;
}

static void
kvl_stop(struct kvl *kvl)
{
	struct circuit *ckt = SIM(kvl)->ckt;

	AG_LockTimeouts(ckt);
	if (AG_TimeoutIsScheduled(ckt, &kvl->update_to)) {
		AG_DelTimeout(ckt, &kvl->update_to);
	}
	AG_UnlockTimeouts(ckt);
}

static void
kvl_destroy(void *p)
{
	struct kvl *kvl = p;
	
	kvl_stop(kvl);

	mat_free(kvl->Rmat);
	vec_free(kvl->Vvec);
	vec_free(kvl->Ivec);
}

static void
kvl_cktmod(void *p, struct circuit *ckt)
{
	struct kvl *kvl = p;

	mat_resize(kvl->Rmat, ckt->l, ckt->l);
	vec_resize(kvl->Vvec, ckt->l);
	vec_resize(kvl->Ivec, ckt->l);
	
	mat_set(kvl->Rmat, 0);
	vec_set(kvl->Vvec, 0);
	vec_set(kvl->Ivec, 0);
}

static void
compose_Rmat(struct kvl *kvl, struct circuit *ckt)
{
	unsigned int m, n, i, j, k;

	for (m = 1; m <= ckt->l; m++) {
		struct cktloop *loop = ckt->loops[m-1];
		struct vsource *vs = (struct vsource *)loop->origin;

		kvl->Vvec->mat[m][0] = vs->voltage;
		kvl->Ivec->mat[m][0] = 0.0;

		for (n = 1; n <= ckt->l; n++)
			kvl->Rmat->mat[m][n] = 0.0;

		for (i = 0; i < loop->ndipoles; i++) {
			struct dipole *dip = loop->dipoles[i];

			for (j = 0; j < dip->nloops; j++) {
				n = dip->loops[j]->name;
#ifdef DEBUG
				if (n > ckt->l)
					fatal("inconsistent loop info");
#endif
				if (n == m) {
					kvl->Rmat->mat[m][n] +=
					    dipole_resistance(dip);
				} else {
					/* XXX */
#if 0
					kvl->Rmat->mat[m][n] -=
					    dipole_resistance(dip);
#else
					for (k = 0; k < dip->nloops; k++) {
						if (j == k) {
							continue;
						}
						if (dip->lpols[j] !=
						    dip->lpols[k]) {
							kvl->Rmat->mat[m][n] -=
							    dipole_resistance(
							    dip);
						}
					}
#endif
				}
			}
		}
	}
}

/* Solve the loop currents of a circuit. */
static int
solve_Ivec(struct kvl *kvl, struct circuit *ckt)
{
	veci_t *ivec;
	vec_t *v;
	mat_t *R;
	double d;

	ivec = veci_new(ckt->l);
	v = vec_new(ckt->l);
	vec_copy(kvl->Vvec, v);

	R = mat_new(ckt->l, ckt->l);
	mat_copy(kvl->Rmat, R);
	if (mat_lu_decompose(R, R, ivec, &d) == NULL) {
		goto fail;
	}
	mat_lu_backsubst(R, ivec, v);
	vec_copy(v, kvl->Ivec);

	vec_free(v);
	veci_free(ivec);
	mat_free(R);
	return (0);
fail:
	vec_free(v);
	veci_free(ivec);
	mat_free(R);
	return (-1);
}

static void
cont_run(int argc, union evarg *argv)
{
	AG_Button *bu = argv[0].p;
	struct kvl *kvl = argv[1].p;
	struct sim *sim = argv[1].p;
	int state = argv[2].i;

	if (state) {
		kvl_cktmod(kvl, sim->ckt);
		kvl_start(kvl);
		sim->running = 1;
		AG_TextTmsg(AG_MSG_INFO, 500, _("Simulation started."));
	} else {
		kvl_stop(kvl);
		sim->running = 0;
		AG_TextTmsg(AG_MSG_INFO, 500, _("Simulation stopped."));
	}
}

static AG_Window *
kvl_edit(void *p, struct circuit *ckt)
{
	struct kvl *kvl = p;
	AG_Window *win;
	AG_Spinbutton *sbu;
	AG_FSpinbutton *fsu;
	AG_Tlist *tl;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;

	win = AG_WindowNew(0);

	nb = AG_NotebookNew(win, AG_NOTEBOOK_WFILL|AG_NOTEBOOK_HFILL);
	ntab = AG_NotebookAddTab(nb, _("Continuous mode"), AG_BOX_VERT);
	{
		AG_ButtonAct(ntab, _("Run simulation"),
		    AG_BUTTON_WFILL|AG_BUTTON_STICKY,
		    cont_run, "%p", kvl);
	
		AG_SeparatorNew(ntab, AG_SEPARATOR_HORIZ);

		sbu = AG_SpinbuttonNew(ntab, _("Updates/sec: "));
		AG_WidgetBind(sbu, "value", AG_WIDGET_UINT32, &kvl->speed);
		AG_SpinbuttonSetRange(sbu, 1, 1000);
		
		AG_LabelNew(ntab, AG_LABEL_POLLED, _("Elapsed time: %[u32]ns"),
		    &kvl->Telapsed);
	}
	ntab = AG_NotebookAddTab(nb, _("R-matrix"), AG_BOX_VERT);
	{
		AG_Matview *mv;

		mv = AG_MatviewNew(ntab, kvl->Rmat, 0);
	}
	return (win);
}

const struct sim_ops kvl_ops = {
	N_("KVL Analysis"),
	EDA_MESH_ICON,
	sizeof(struct kvl),
	kvl_init,
	kvl_destroy,
	kvl_cktmod,
	kvl_edit
};
