/*	$Csoft: vsource.c,v 1.3 2005/09/10 05:48:21 vedge Exp $	*/

/*
 * Copyright (c) 2004, 2005 CubeSoft Communications, Inc.
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
#include <engine/vg/vg.h>

#ifdef EDITION
#include <engine/widget/window.h>
#include <engine/widget/fspinbutton.h>
#include <engine/widget/label.h>
#include <engine/widget/tlist.h>
#endif

#include "vsource.h"

const struct version vsource_ver = {
	"agar-eda voltage source",
	0, 0
};

const struct component_ops vsource_ops = {
	{
		vsource_init,
		vsource_reinit,
		vsource_destroy,
		vsource_load,
		vsource_save,
		component_edit
	},
	N_("Independent voltage source"),
	"V",
	vsource_draw,
	vsource_edit,
	NULL,
	vsource_export,
	vsource_tick,
	NULL,			/* resistance */
	NULL,			/* capacitance */
	NULL,			/* inductance */
	NULL			/* isource */
};

const struct pin vsource_pinout[] = {
	{ 0, "",  0, 0 },
	{ 1, "v+", 0, 0 },
	{ 2, "v-", 0, 2 },
	{ -1 },
};

enum {
	VSOURCE_SYM_LINEAR,
	VSOURCE_SYM_CIRCULAR
} vsource_sym = 1;

void
vsource_draw(void *p, struct vg *vg)
{
	struct vsource *vs = p;
	
	switch (vsource_sym) {
	case VSOURCE_SYM_LINEAR:
		vg_begin_element(vg, VG_LINES);
		vg_vertex2(vg, 0.0000, 0.0000);
		vg_vertex2(vg, 0.0000, 0.3125);
		vg_vertex2(vg, 0.0000, 0.6875);
		vg_vertex2(vg, 0.0000, 1.0000);
		vg_vertex2(vg, -0.0937, 0.3125);
		vg_vertex2(vg, +0.0937, 0.3125);
		vg_vertex2(vg, -0.1875, 0.4375);
		vg_vertex2(vg, +0.1875, 0.4375);
		vg_vertex2(vg, -0.0937, 0.5625);
		vg_vertex2(vg, +0.0937, 0.5625);
		vg_vertex2(vg, -0.1875, 0.6875);
		vg_vertex2(vg, +0.1875, 0.6875);
		vg_end_element(vg);

		vg_begin_element(vg, VG_TEXT);
		vg_style(vg, "component-name");
		vg_vertex2(vg, -0.0468, 0.0625);
		vg_text_align(vg, VG_ALIGN_TR);
		vg_printf(vg, "%s\n%.2fV\n", OBJECT(vs)->name, vs->voltage);
		vg_end_element(vg);
		break;
	case VSOURCE_SYM_CIRCULAR:
		vg_begin_element(vg, VG_LINES);
		vg_vertex2(vg, 0.000, 0.000);
		vg_vertex2(vg, 0.000, 0.400);
		vg_vertex2(vg, 0.000, 1.600);
		vg_vertex2(vg, 0.000, 2.000);
		vg_end_element(vg);

		vg_begin_element(vg, VG_CIRCLE);
		vg_vertex2(vg, 0.0, 1.0);
		vg_circle_radius(vg, 0.6);
		vg_end_element(vg);

		vg_begin_element(vg, VG_TEXT);
		vg_style(vg, "component-name");
		vg_vertex2(vg, 0.0, 1.0);
		vg_text_align(vg, VG_ALIGN_MC);
		vg_printf(vg, "%s", OBJECT(vs)->name);
		vg_end_element(vg);
		
		vg_begin_element(vg, VG_TEXT);
		vg_style(vg, "component-name");
		vg_vertex2(vg, 0.0, 0.6);
		vg_text_align(vg, VG_ALIGN_MC);
		vg_printf(vg, "+");
		vg_end_element(vg);
		
		vg_begin_element(vg, VG_TEXT);
		vg_style(vg, "component-name");
		vg_vertex2(vg, 0.0, 1.4);
		vg_text_align(vg, VG_ALIGN_MC);
		vg_printf(vg, "-");
		vg_end_element(vg);
		break;
	}
}

/* Find the dipole connecting two contiguous pins, and its polarity. */
static int
contig_dipole(struct pin *pA, struct pin *pB, struct dipole **Rdip,
    int *Rpol)
{
	struct component *com = pA->com;
	struct circuit *ckt = com->ckt;
	unsigned int i;

	for (i = 0; i < com->ndipoles; i++) {
		struct dipole *dip = &com->dipoles[i];
		struct cktnode *node;
		struct cktbranch *br;
		int pol;

		/*
		 * As a convention, the relative polarities of dipoles is
		 * determined by the pin number order.
		 */
		if (dip->p1 == pA && dip->p2->node != NULL) {
			node = ckt->nodes[dip->p2->node];
			pol = -1;
		} else if (dip->p2 == pA && dip->p1->node != NULL) {
			node = ckt->nodes[dip->p1->node];
			pol = +1;
		} else {
			continue;
		}

		TAILQ_FOREACH(br, &node->branches, branches) {
			if (br->pin == pB)
				break;
		}
		if (br != NULL) {
			*Rdip = dip;
			*Rpol = pol;
			break;
		}
	}
	return (i < com->ndipoles);
}

/*
 * Insert a new voltage source loop based on the pins currently in the
 * loop stack.
 */
static void
insert_loop(struct vsource *vs)
{
	struct cktloop *lnew;
	unsigned int i;

	/* Create a new voltage source loop entry. */
	lnew = Malloc(sizeof(struct cktloop), M_EDA);
	lnew->name = 1+vs->nloops++;
	lnew->dipoles = Malloc((vs->nlstack-1)*sizeof(struct dipole *), M_EDA);
	lnew->ndipoles = 0;
	lnew->origin = vs;

	for (i = 1; i < vs->nlstack; i++) {
		struct dipole *dip;
		int pol;

		if (!contig_dipole(vs->lstack[i], vs->lstack[i-1], &dip,
		    &pol))
			continue;

		lnew->dipoles[lnew->ndipoles++] = dip;

		dip->loops = Realloc(dip->loops,
		    (dip->nloops+1)*sizeof(struct cktloop *));
		dip->lpols = Realloc(dip->lpols,
		    (dip->nloops+1)*sizeof(int));
		dip->loops[dip->nloops] = lnew;
		dip->lpols[dip->nloops] = pol;
		dip->nloops++;
	}
	TAILQ_INSERT_TAIL(&vs->loops, lnew, loops);
}

/*
 * Isolate closed loops with respect to a voltage source.
 *
 * The resulting loop structure is an array of dipoles whose polarities
 * with respect to the loop are recorded in a separate array.
 */
static void
find_loops(struct vsource *vs, struct pin *pcur)
{
	struct circuit *ckt = COM(vs)->ckt;
	struct cktnode *node = ckt->nodes[pcur->node];
	struct cktbranch *br;
	unsigned int i;

	node->flags |= CKTNODE_EXAM;

	vs->lstack = Realloc(vs->lstack, (vs->nlstack+1)*sizeof(struct pin *));
	vs->lstack[vs->nlstack++] = pcur;

	TAILQ_FOREACH(br, &node->branches, branches) {
		if (br->pin == pcur) {
			continue;
		}
		if (br->pin->com == COM(vs) &&
		    br->pin->n == 2 &&
		    vs->nlstack > 2) {
			vs->lstack = Realloc(vs->lstack,
			    (vs->nlstack+1)*sizeof(struct pin *));
			vs->lstack[vs->nlstack++] = &COM(vs)->pin[2];
			insert_loop(vs);
			vs->nlstack--;
			continue;
		}
		for (i = 1; i <= br->pin->com->npins; i++) {
			struct pin *pnext = &br->pin->com->pin[i];
			struct cktnode *nnext = ckt->nodes[pnext->node];

			if ((pnext == pcur) ||
			    (pnext->com == br->pin->com &&
			     pnext->n == br->pin->n) ||
			    (nnext->flags & CKTNODE_EXAM)) {
				continue;
			}
			find_loops(vs, pnext);
		}
	}

	vs->nlstack--;
	node->flags &= ~(CKTNODE_EXAM);
}

/* Isolate the loops relative to the given voltage source. */
void
vsource_find_loops(struct vsource *vs)
{
	unsigned int i;

	vs->lstack = Malloc(sizeof(struct pin *), M_EDA);
	find_loops(vs, PIN(vs,1));

	Free(vs->lstack, M_EDA);
	vs->lstack = NULL;
	vs->nlstack = 0;
}

void
vsource_tick(void *p)
{
	struct vsource *vs = p;
	struct cktnode *n1, *n2;
	struct circuit *ckt = COM(vs)->ckt;

	if (PNODE(vs,1) == -1 ||
	    PNODE(vs,2) == -1)
		return;

	n1 = ckt->nodes[PNODE(vs,1)];
	n2 = ckt->nodes[PNODE(vs,2)];

	/* XXX */
	if (n1->nbranches >= 2 && n2->nbranches >= 2) {
		U(vs,1) = vs->voltage;
		U(vs,2) = 0.0;
	}
}

void
vsource_init(void *p, const char *name)
{
	struct vsource *vs = p;

	component_init(vs, "vsource", name, &vsource_ops, vsource_pinout);
	vs->voltage = 5;
	vs->lstack = NULL;
	vs->nlstack = 0;
	TAILQ_INIT(&vs->loops);
	vs->nloops = 0;
}

void
vsource_reinit(void *p)
{
	struct vsource *vs = p;

	vsource_free_loops(vs);
}

void
vsource_destroy(void *p)
{
	struct vsource *vs = p;

	vsource_free_loops(vs);
}

void
vsource_free_loops(struct vsource *vs)
{
	struct cktloop *loop, *nloop;
	unsigned int i;

	for (loop = TAILQ_FIRST(&vs->loops);
	     loop != TAILQ_END(&vs->loops);
	     loop = nloop) {
		nloop = TAILQ_NEXT(loop, loops);
		Free(loop->dipoles, M_EDA);
		Free(loop, M_EDA);
	}
	TAILQ_INIT(&vs->loops);
	vs->nloops = 0;

	Free(vs->lstack, M_EDA);
	vs->lstack = NULL;
	vs->nlstack = 0;
}

int
vsource_load(void *p, struct netbuf *buf)
{
	struct vsource *vs = p;

	if (version_read(buf, &vsource_ver, NULL) == -1 ||
	    component_load(vs, buf) == -1)
		return (-1);

	vs->voltage = read_double(buf);
	return (0);
}

int
vsource_save(void *p, struct netbuf *buf)
{
	struct vsource *vs = p;

	version_write(buf, &vsource_ver);
	if (component_save(vs, buf) == -1)
		return (-1);

	write_double(buf, vs->voltage);
	return (0);
}

int
vsource_export(void *p, enum circuit_format fmt, FILE *f)
{
	struct vsource *vs = p;

	if (PNODE(vs,1) == -1 ||
	    PNODE(vs,2) == -1)
		return (0);

	switch (fmt) {
	case CIRCUIT_SPICE3:
		fprintf(f, "%s %d %d %g\n", OBJECT(vs)->name,
		    PNODE(vs,1), PNODE(vs,2), vs->voltage);
		break;
	}
	return (0);
}

double
vsource_emf(void *p, int pin1, int pin2)
{
	struct vsource *vs = p;

	return (pin1 == 1 ? vs->voltage : -(vs->voltage));
}

#ifdef EDITION
static void
poll_loops(int argc, union evarg *argv)
{
	char text[TLIST_LABEL_MAX];
	struct tlist *tl = argv[0].p;
	struct vsource *vs = argv[1].p;
	struct tlist_item *it;
	struct cktloop *l;
	unsigned int i, j;

	tlist_clear_items(tl);
	TAILQ_FOREACH(l, &vs->loops, loops) {
		snprintf(text, sizeof(text), "L%u", l->name);
		it = tlist_insert_item(tl, ICON(EDA_MESH_ICON), text, l);
		it->depth = 0;

		for (i = 0; i < l->ndipoles; i++) {
			struct dipole *dip = l->dipoles[i];

			snprintf(text, sizeof(text), "%s:(%s,%s)",
			    OBJECT(dip->com)->name, dip->p1->name,
			    dip->p2->name);
			it = tlist_insert_item(tl, ICON(EDA_NODE_ICON), text,
			    dip);
			it->depth = 1;
#if 0
			for (j = 0; j < dip->nloops; j++) {
				struct cktloop *dloop = dip->loops[j];
				int dpol = dip->lpols[j];

				snprintf(text, sizeof(text), "%s:L%d (%s)",
				    OBJECT(dloop->origin)->name, dloop->name,
				    dpol == 1 ? "+" : "-");
				it = tlist_insert_item(tl, NULL, text,
				    &dip->loops[j]);
				it->depth = 2;
			}
#endif
		}
	}
	tlist_restore_selections(tl);
}

struct window *
vsource_edit(void *p)
{
	struct vsource *vs = p;
	struct window *win, *subwin;
	struct spinbutton *sb;
	struct fspinbutton *fsb;
	struct tlist *tl;

	win = window_new(0, NULL);

	fsb = fspinbutton_new(win, "V", _("Voltage: "));
	widget_bind(fsb, "value", WIDGET_DOUBLE, &vs->voltage);
	
	label_new(win, LABEL_STATIC, _("Loops:"));
	tl = tlist_new(win, TLIST_POLL|TLIST_TREE);
	event_new(tl, "tlist-poll", poll_loops, "%p", vs);
	
	return (win);
}
#endif /* EDITION */
