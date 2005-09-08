/*	$Csoft: dc.c,v 1.2 2004/10/01 03:10:34 vedge Exp $	*/

/*
 * Copyright (c) 2005 Winds Triton Engineering, Inc.
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
#ifdef EDITION
#include <engine/widget/window.h>
#include <engine/widget/spinbutton.h>
#include <engine/widget/fspinbutton.h>
#include <engine/widget/tlist.h>
#include <engine/widget/label.h>
#endif
#include <circuit/circuit.h>
#include <component/component.h>
#include <component/vsource.h>
#include <mat/mat.h>

#include "analysis.h"

const struct analysis_ops kvl_ops;

struct kvl {
	struct analysis an;
	Uint32 Telapsed;		/* Simulated time elapsed (ns) */
	int speed;			/* Simulation speed (updates/sec) */
	struct timeout update_to;	/* Timer for simulation updates */
	
	struct mat *Rmat;		/* Resistance matrix */
	struct vec *Vvec;		/* Independent voltages */
	struct vec *Ivec;		/* Vector of unknown currents */
};

static void compose_Rmat(struct kvl *, struct circuit *);
static int solve_Ivec(struct kvl *, struct circuit *);

static Uint32
kvl_tick(void *obj, Uint32 ival, void *arg)
{
	struct circuit *ckt = obj;
	struct analysis *an = arg;
	struct kvl *kvl = arg;
	struct component *com;

	/* Effect the independent voltage sources. */
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!OBJECT_TYPE(com, "vsource")) {
			continue;
		}
		if (com->ops->tick != NULL)
			com->ops->tick(com);
	}

	/* Update model states. */
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (OBJECT_TYPE(com, "map")) {
			continue;
		}
		if (com->ops->tick != NULL)
			com->ops->tick(com);
	}

	/* Solve the loop currents. */
	compose_Rmat(kvl, ckt);
	if (solve_Ivec(kvl, ckt) == -1)
		fprintf(stderr, "%s: %s\n", OBJECT(ckt)->name, error_get());

	kvl->Telapsed++;
	return (ival);
}

static void
kvl_init(void *p)
{
	struct kvl *kvl = p;
	struct analysis *an = p;

	analysis_init(kvl, &kvl_ops);
	kvl->Telapsed = 0;
	kvl->speed = 4;
	kvl->Rmat = mat_new(0, 0);
	kvl->Vvec = vec_new(0);
	kvl->Ivec = vec_new(0);
	timeout_set(&kvl->update_to, kvl_tick, an, TIMEOUT_LOADABLE);
}

static void
kvl_destroy(void *p)
{
	struct kvl *kvl = p;
	struct analysis *an = p;
	struct circuit *ckt = an->ckt;

	mat_free(kvl->Rmat);
	vec_free(kvl->Vvec);
	vec_free(kvl->Ivec);

	analysis_destroy(kvl);
}

static void
kvl_start(void *p)
{
	struct kvl *kvl = p;
	struct analysis *an = p;

	lock_timeout(an->ckt);
	if (timeout_scheduled(an->ckt, &kvl->update_to)) {
		timeout_del(an->ckt, &kvl->update_to);
	}
	timeout_add(an->ckt, &kvl->update_to, 1000/kvl->speed);
	unlock_timeout(an->ckt);

	kvl->Telapsed = 0;
}

static void
kvl_stop(void *p)
{
	struct kvl *kvl = p;
	struct analysis *an = p;

	lock_timeout(an->ckt);
	if (timeout_scheduled(an->ckt, &kvl->update_to)) {
		timeout_del(an->ckt, &kvl->update_to);
	}
	unlock_timeout(an->ckt);
}

static void
kvl_cktmod(void *p)
{
	struct kvl *kvl = p;
	struct analysis *an = p;
	struct circuit *ckt = an->ckt;

	mat_resize(kvl->Rmat, ckt->nloops, ckt->nloops);
	vec_resize(kvl->Vvec, ckt->nloops);
	vec_resize(kvl->Ivec, ckt->nloops);
	
	mat_set(kvl->Rmat, 0);
	vec_set(kvl->Vvec, 0);
	vec_set(kvl->Ivec, 0);
}

static void
compose_Rmat(struct kvl *kvl, struct circuit *ckt)
{
	unsigned int m, n, i, j, k;

	for (m = 1; m <= ckt->nloops; m++) {
		struct cktloop *loop = ckt->loops[m-1];
		struct vsource *vs = (struct vsource *)loop->origin;

		kvl->Vvec->vec[m] = vs->voltage;
		kvl->Ivec->vec[m] = 0.0;

		for (n = 1; n <= ckt->nloops; n++)
			kvl->Rmat->mat[m][n] = 0.0;

		for (i = 0; i < loop->ndipoles; i++) {
			struct dipole *dip = loop->dipoles[i];

			for (j = 0; j < dip->nloops; j++) {
				n = dip->loops[j]->name;
#ifdef DEBUG
				if (n > ckt->nloops)
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
	struct veci *ivec;
	struct vec *v;
	struct mat *R;
	double d;

	ivec = veci_new(ckt->nloops);
	v = vec_new(ckt->nloops);
	vec_copy(kvl->Vvec, v);

	R = mat_new(ckt->nloops, ckt->nloops);
	mat_copy(kvl->Rmat, R);
	if (mat_lu_decompose(R, ivec, &d) == -1) {
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
poll_Rmat(int argc, union evarg *argv)
{
	char text[TLIST_LABEL_MAX];
	struct tlist *tl = argv[0].p;
	struct kvl *kvl = argv[1].p;
	struct analysis *an = argv[1].p;
	struct circuit *ckt = an->ckt;
	unsigned int i, j;

	if (kvl->Rmat->mat == NULL)
		return;

	tlist_clear_items(tl);
	for (i = 1; i <= kvl->Rmat->m; i++) {
		text[0] = '\0';
		snprintf(text, sizeof(text), "%.0fV ", kvl->Vvec->vec[i]);
		for (j = 1; j <= kvl->Rmat->n; j++) {
			char tmp[16];

			snprintf(tmp, sizeof(tmp), "%.0f ",
			    kvl->Rmat->mat[i][j]);
			strlcat(text, tmp, sizeof(text));
		}
		tlist_insert_item(tl, NULL, text, &kvl->Rmat->mat[i][j]);
	}
	tlist_restore_selections(tl);
}

static struct window *
kvl_edit(void *p)
{
	char path[OBJECT_PATH_MAX];
	struct analysis *an = p;
	struct kvl *kvl = p;
	struct circuit *ckt = an->ckt;
	struct window *win;
	struct spinbutton *sbu;
	struct fspinbutton *fsu;
	struct tlist *tl;

	object_copy_name(ckt, path, sizeof(path));
	if ((win = window_new(0, "kvl-settings-%s", path)) == NULL) {
		return (NULL);
	}
	window_set_caption(win, _("KVL analysis: %s"), OBJECT(ckt)->name);
	window_set_position(win, WINDOW_LOWER_CENTER, 0);

	label_new(win, LABEL_POLLED, _("Elapsed time: %[u32]ns"),
	    &kvl->Telapsed);

	sbu = spinbutton_new(win, _("Speed (updates/sec): "));
	widget_bind(sbu, "value", WIDGET_UINT32, &kvl->speed);
	spinbutton_set_range(sbu, 1, 1000);
	
	label_new(win, LABEL_STATIC, _("Resistances:"));
	tl = tlist_new(win, TLIST_POLL);
	event_new(tl, "tlist-poll", poll_Rmat, "%p", kvl);

	return (win);
}

const struct analysis_ops kvl_ops = {
	N_("KVL analysis"),
	sizeof(struct kvl),
	kvl_init,
	kvl_destroy,
	kvl_start,
	kvl_stop,
	kvl_cktmod,
	kvl_edit
};
