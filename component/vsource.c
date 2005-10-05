/*	$Csoft: vsource.c,v 1.5 2005/09/27 03:34:09 vedge Exp $	*/

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
#include <engine/widget/spinbutton.h>
#include <engine/widget/fspinbutton.h>
#include <engine/widget/label.h>
#include <engine/widget/tlist.h>
#endif

#include "vsource.h"

const AG_Version vsource_ver = {
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
vsource_draw(void *p, VG *vg)
{
	struct vsource *vs = p;
	
	switch (vsource_sym) {
	case VSOURCE_SYM_LINEAR:
		VG_Begin(vg, VG_LINES);
		VG_Vertex2(vg, 0.0000, 0.0000);
		VG_Vertex2(vg, 0.0000, 0.3125);
		VG_Vertex2(vg, 0.0000, 0.6875);
		VG_Vertex2(vg, 0.0000, 1.0000);
		VG_Vertex2(vg, -0.0937, 0.3125);
		VG_Vertex2(vg, +0.0937, 0.3125);
		VG_Vertex2(vg, -0.1875, 0.4375);
		VG_Vertex2(vg, +0.1875, 0.4375);
		VG_Vertex2(vg, -0.0937, 0.5625);
		VG_Vertex2(vg, +0.0937, 0.5625);
		VG_Vertex2(vg, -0.1875, 0.6875);
		VG_Vertex2(vg, +0.1875, 0.6875);
		VG_End(vg);

		VG_Begin(vg, VG_TEXT);
		VG_SetStyle(vg, "component-name");
		VG_Vertex2(vg, -0.0468, 0.0625);
		VG_TextAlignment(vg, VG_ALIGN_TR);
		VG_Printf(vg, "%s\n%.2fV\n", AGOBJECT(vs)->name, vs->voltage);
		VG_End(vg);
		break;
	case VSOURCE_SYM_CIRCULAR:
		VG_Begin(vg, VG_LINES);
		VG_Vertex2(vg, 0.000, 0.000);
		VG_Vertex2(vg, 0.000, 0.400);
		VG_Vertex2(vg, 0.000, 1.600);
		VG_Vertex2(vg, 0.000, 2.000);
		VG_End(vg);

		VG_Begin(vg, VG_CIRCLE);
		VG_Vertex2(vg, 0.0, 1.0);
		VG_CircleRadius(vg, 0.6);
		VG_End(vg);

		VG_Begin(vg, VG_TEXT);
		VG_SetStyle(vg, "component-name");
		VG_Vertex2(vg, 0.0, 1.0);
		VG_TextAlignment(vg, VG_ALIGN_MC);
		VG_Printf(vg, "%s", AGOBJECT(vs)->name);
		VG_End(vg);
		
		VG_Begin(vg, VG_TEXT);
		VG_SetStyle(vg, "component-name");
		VG_Vertex2(vg, 0.0, 0.6);
		VG_TextAlignment(vg, VG_ALIGN_MC);
		VG_Printf(vg, "+");
		VG_End(vg);
		
		VG_Begin(vg, VG_TEXT);
		VG_SetStyle(vg, "component-name");
		VG_Vertex2(vg, 0.0, 1.4);
		VG_TextAlignment(vg, VG_ALIGN_MC);
		VG_Printf(vg, "-");
		VG_End(vg);
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
vsource_load(void *p, AG_Netbuf *buf)
{
	struct vsource *vs = p;

	if (AG_ReadVersion(buf, &vsource_ver, NULL) == -1 ||
	    component_load(vs, buf) == -1)
		return (-1);

	vs->voltage = AG_ReadDouble(buf);
	return (0);
}

int
vsource_save(void *p, AG_Netbuf *buf)
{
	struct vsource *vs = p;

	AG_WriteVersion(buf, &vsource_ver);
	if (component_save(vs, buf) == -1)
		return (-1);

	AG_WriteDouble(buf, vs->voltage);
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
		fprintf(f, "%s %d %d %g\n", AGOBJECT(vs)->name,
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
	char text[AG_TLIST_LABEL_MAX];
	AG_Tlist *tl = argv[0].p;
	struct vsource *vs = argv[1].p;
	AG_TlistItem *it;
	struct cktloop *l;
	unsigned int i, j;

	AG_TlistClear(tl);
	TAILQ_FOREACH(l, &vs->loops, loops) {
		snprintf(text, sizeof(text), "L%u", l->name);
		it = AG_TlistAddPtr(tl, AGICON(EDA_MESH_ICON), text, l);
		it->depth = 0;

		for (i = 0; i < l->ndipoles; i++) {
			struct dipole *dip = l->dipoles[i];

			snprintf(text, sizeof(text), "%s:(%s,%s)",
			    AGOBJECT(dip->com)->name, dip->p1->name,
			    dip->p2->name);
			it = AG_TlistAddPtr(tl, AGICON(EDA_NODE_ICON), text,
			    dip);
			it->depth = 1;
#if 0
			for (j = 0; j < dip->nloops; j++) {
				struct cktloop *dloop = dip->loops[j];
				int dpol = dip->lpols[j];

				snprintf(text, sizeof(text), "%s:L%d (%s)",
				    AGOBJECT(dloop->origin)->name, dloop->name,
				    dpol == 1 ? "+" : "-");
				it = AG_TlistAddPtr(tl, NULL, text,
				    &dip->loops[j]);
				it->depth = 2;
			}
#endif
		}
	}
	AG_TlistRestore(tl);
}

AG_Window *
vsource_edit(void *p)
{
	struct vsource *vs = p;
	AG_Window *win, *subwin;
	AG_Spinbutton *sb;
	AG_FSpinbutton *fsb;
	AG_Tlist *tl;

	win = AG_WindowNew(0);

	fsb = AG_FSpinbuttonNew(win, "V", _("Voltage: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &vs->voltage);
	
	AG_LabelNew(win, AG_LABEL_STATIC, _("Loops:"));
	tl = AG_TlistNew(win, AG_TLIST_POLL|AG_TLIST_TREE);
	AG_SetEvent(tl, "tlist-poll", poll_loops, "%p", vs);
	
	return (win);
}
#endif /* EDITION */
