/*
 * Copyright (c) 2004-2005 Hypertriton, Inc. <http://hypertriton.com/>
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
#include <agar/vg.h>
#include <agar/gui.h>

#include "eda.h"
#include "vsource.h"

const ES_ComponentOps esVsourceOps = {
	{
		"ES_Component:ES_Vsource",
		sizeof(ES_Vsource),
		{ 0,0 },
		ES_VsourceInit,
		ES_VsourceReinit,
		ES_VsourceDestroy,
		ES_VsourceLoad,
		ES_VsourceSave,
		ES_ComponentEdit
	},
	N_("Independent voltage source"),
	"V",
	ES_VsourceDraw,
	ES_VsourceEdit,
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	ES_VsourceExport,
	NULL			/* connect */
};

const ES_Port esVsourcePinout[] = {
	{ 0, "",  0, 0 },
	{ 1, "v+", 0, 0 },
	{ 2, "v-", 0, 2 },
	{ -1 },
};

enum {
	VSOURCE_SYM_LINEAR,
	VSOURCE_SYM_CIRCULAR
} esVsourceStyle = 1;

void
ES_VsourceDraw(void *p, VG *vg)
{
	ES_Vsource *vs = p;
	
	switch (esVsourceStyle) {
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

/* Find the pairs connecting two contiguous ports, and its polarity. */
static int
contig_pair(ES_Port *pA, ES_Port *pB, ES_Pair **Rdip, int *Rpol)
{
	ES_Component *com = pA->com;
	ES_Circuit *ckt = com->ckt;
	unsigned int i;

	for (i = 0; i < com->npairs; i++) {
		ES_Pair *dip = &com->pairs[i];
		ES_Node *node;
		ES_Branch *br;
		int pol;

		/*
		 * As a convention, the relative polarities of pairs is
		 * determined by the port number order.
		 */
		if (dip->p1 == pA && dip->p2->node >= 0) {
			node = ckt->nodes[dip->p2->node];
			pol = -1;
		} else if (dip->p2 == pA && dip->p1->node >= 0) {
			node = ckt->nodes[dip->p1->node];
			pol = +1;
		} else {
			continue;
		}

		TAILQ_FOREACH(br, &node->branches, branches) {
			if (br->port == pB)
				break;
		}
		if (br != NULL) {
			*Rdip = dip;
			*Rpol = pol;
			break;
		}
	}
	return (i < com->npairs);
}

/*
 * Insert a new voltage source loop based on the ports currently in the
 * loop stack.
 */
static void
insert_loop(ES_Vsource *vs)
{
	ES_Loop *lnew;
	unsigned int i;

	/* Create a new voltage source loop entry. */
	lnew = Malloc(sizeof(ES_Loop));
	lnew->name = 1+vs->nloops++;
	lnew->pairs = Malloc((vs->nlstack-1)*sizeof(ES_Pair *));
	lnew->npairs = 0;
	lnew->origin = vs;

	for (i = 1; i < vs->nlstack; i++) {
		ES_Pair *dip;
		int pol;

		if (!contig_pair(vs->lstack[i], vs->lstack[i-1], &dip, &pol))
			continue;

		lnew->pairs[lnew->npairs++] = dip;

		dip->loops = Realloc(dip->loops,
		    (dip->nloops+1)*sizeof(ES_Loop *));
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
 * The resulting loop structure is an array of pairs whose polarities
 * with respect to the loop are recorded in a separate array.
 */
static void
find_loops(ES_Vsource *vs, ES_Port *pcur)
{
	ES_Circuit *ckt = COM(vs)->ckt;
	ES_Node *node = ckt->nodes[pcur->node];
	ES_Branch *br;
	unsigned int i;

	node->flags |= CKTNODE_EXAM;

	vs->lstack = Realloc(vs->lstack, (vs->nlstack+1)*sizeof(ES_Port *));
	vs->lstack[vs->nlstack++] = pcur;

	TAILQ_FOREACH(br, &node->branches, branches) {
		if (br->port == pcur) {
			continue;
		}
		if (br->port->com == COM(vs) &&
		    br->port->n == 2 &&
		    vs->nlstack > 2) {
			vs->lstack = Realloc(vs->lstack,
			    (vs->nlstack+1)*sizeof(ES_Port *));
			vs->lstack[vs->nlstack++] = &COM(vs)->ports[2];
			insert_loop(vs);
			vs->nlstack--;
			continue;
		}
		if (br->port->com == NULL) {
			continue;
		}
		for (i = 1; i <= br->port->com->nports; i++) {
			ES_Port *pnext = &br->port->com->ports[i];
			ES_Node *nnext = ckt->nodes[pnext->node];

			if ((pnext == pcur) ||
			    (pnext->com == br->port->com &&
			     pnext->n == br->port->n) ||
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
ES_VsourceFindLoops(ES_Vsource *vs)
{
	unsigned int i;

	vs->lstack = Malloc(sizeof(ES_Port *));
	find_loops(vs, PORT(vs,1));

	Free(vs->lstack);
	vs->lstack = NULL;
	vs->nlstack = 0;
}

/*
 * Ideal voltage sources require the addition of one unknown (iE), and a
 * branch current equation (br). Voltage sources contribute two entries to
 * the B/C matrices and one entry to the right-hand side vector e.
 *
 *    |  Vk  Vj  iE  | RHS
 *    |--------------|-----
 *  k |           1  |
 *  j |          -1  |
 * br | 1   -1       | Ekj
 *    |--------------|-----
 */
static int
LoadDC_BCD(void *p, SC_Matrix *B, SC_Matrix *C, SC_Matrix *D)
{
	ES_Vsource *vs = p;
	u_int k = PNODE(vs,1);
	u_int j = PNODE(vs,2);
	int v;

	if ((v = ES_VsourceName(vs)) == -1) {
		AG_SetError("no such vsource");
		return (-1);
	}
	if (k != 0) {
		mSet(B,k,v, 1.0);
		mSet(C,v,k, 1.0);
	}
	if (j != 0) {
		mSet(B,j,v, -1.0);
		mSet(C,v,j, -1.0);
	}
	return (0);
}
static int
LoadDC_RHS(void *p, SC_Vector *i, SC_Vector *e)
{
	ES_Vsource *vs = p;
	int v;

	if ((v = ES_VsourceName(vs)) == -1 ) {
		AG_SetError("no such vsource");
		return (-1);
	}
	vSet(e,v, vs->voltage);
	return (0);
}

static void
Connected(AG_Event *event)
{
	ES_Circuit *ckt = AG_SENDER();
	ES_Vsource *vs = AG_SELF();

	ckt->vsrcs = Realloc(ckt->vsrcs, (ckt->m+1)*sizeof(ES_Vsource *));
	ckt->vsrcs[ckt->m] = vs;
	ckt->m++;

	ES_CircuitLog(ckt, "added vsource: %d", ckt->m-1);
}

static void
Disconnected(AG_Event *event)
{
	ES_Circuit *ckt = AG_SENDER();
	ES_Vsource *vs = AG_SELF();
	u_int i, j;
	
	for (i = 0; i < ckt->m; i++) {
		if (ckt->vsrcs[i] == vs) {
			if (i < --ckt->m) {
				for (; i < ckt->m; i++)
					ckt->vsrcs[i] = ckt->vsrcs[i+1];
			}
			ES_CircuitLog(ckt, "removed vsource: %d", i);
			break;
		}
	}
}

void
ES_VsourceInit(void *p, const char *name)
{
	ES_Vsource *vs = p;

	ES_ComponentInit(vs, name, &esVsourceOps, esVsourcePinout);
	vs->voltage = 5;
	vs->lstack = NULL;
	vs->nlstack = 0;
	TAILQ_INIT(&vs->loops);
	vs->nloops = 0;

	COM(vs)->loadDC_BCD = LoadDC_BCD;
	COM(vs)->loadDC_RHS = LoadDC_RHS;
	AG_SetEvent(vs, "circuit-connected", Connected, NULL);
	AG_SetEvent(vs, "circuit-disconnected", Disconnected, NULL);
}

void
ES_VsourceReinit(void *p)
{
	ES_Vsource *vs = p;

	ES_VsourceFreeLoops(vs);
}

void
ES_VsourceDestroy(void *p)
{
	ES_Vsource *vs = p;

	ES_VsourceFreeLoops(vs);
}

void
ES_VsourceFreeLoops(ES_Vsource *vs)
{
	ES_Loop *loop, *nloop;
	unsigned int i;

	for (loop = TAILQ_FIRST(&vs->loops);
	     loop != TAILQ_END(&vs->loops);
	     loop = nloop) {
		nloop = TAILQ_NEXT(loop, loops);
		Free(loop->pairs);
		Free(loop);
	}
	TAILQ_INIT(&vs->loops);
	vs->nloops = 0;

	Free(vs->lstack);
	vs->lstack = NULL;
	vs->nlstack = 0;
}

int
ES_VsourceLoad(void *p, AG_DataSource *buf)
{
	ES_Vsource *vs = p;

	if (AG_ReadObjectVersion(buf, vs, NULL) == -1 ||
	    ES_ComponentLoad(vs, buf) == -1)
		return (-1);

	vs->voltage = AG_ReadDouble(buf);
	return (0);
}

int
ES_VsourceSave(void *p, AG_DataSource *buf)
{
	ES_Vsource *vs = p;

	AG_WriteObjectVersion(buf, vs);
	if (ES_ComponentSave(vs, buf) == -1)
		return (-1);

	AG_WriteDouble(buf, vs->voltage);
	return (0);
}

int
ES_VsourceExport(void *p, enum circuit_format fmt, FILE *f)
{
	ES_Vsource *vs = p;

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
ES_VsourceVoltage(void *p, int p1, int p2)
{
	ES_Vsource *vs = p;

	return (p1 == 1 ? vs->voltage : -(vs->voltage));
}

int
ES_VsourceName(ES_Vsource *vs)
{
	ES_Circuit *ckt = COM(vs)->ckt;
	int i;

	for (i = 1; i <= ckt->m; i++) {
		if (VSOURCE(ckt->vsrcs[i-1]) == vs)
			return (i);
	}
	return (-1);
}

#ifdef EDITION
static void
PollLoops(AG_Event *event)
{
	char text[AG_TLIST_LABEL_MAX];
	ES_Vsource *vs = AG_PTR(1);
	AG_Tlist *tl = AG_SELF();
	AG_TlistItem *it;
	ES_Loop *l;
	unsigned int i, j;
	int k;

	AG_TlistClear(tl);

	k = ES_VsourceName(vs);
	it = AG_TlistAdd(tl, NULL, "I(%d)=%.04fA", k,
	    ES_BranchCurrent(COM(vs)->ckt, k));
	it->depth = 0;

	TAILQ_FOREACH(l, &vs->loops, loops) {
		it = AG_TlistAdd(tl, NULL, "L%u", l->name);
		it->p1 = l;
		it->depth = 0;

		for (i = 0; i < l->npairs; i++) {
			ES_Pair *dip = l->pairs[i];

			snprintf(text, sizeof(text), "%s:(%s,%s)",
			    AGOBJECT(dip->com)->name, dip->p1->name,
			    dip->p2->name);
			it = AG_TlistAddPtr(tl, NULL, text,
			    dip);
			it->depth = 1;
		}
	}
	AG_TlistRestore(tl);
}

void *
ES_VsourceEdit(void *p)
{
	ES_Vsource *vs = p;
	AG_Window *win, *subwin;
	AG_Spinbutton *sb;
	AG_FSpinbutton *fsb;
	AG_Tlist *tl;

	win = AG_WindowNew(0);

	fsb = AG_FSpinbuttonNew(win, 0, "V", _("Voltage: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &vs->voltage);
	
	AG_LabelNewStatic(win, 0, _("Loops:"));
	tl = AG_TlistNewPolled(win, AG_TLIST_TREE|AG_TLIST_EXPAND,
	    PollLoops, "%p", vs);
	
	return (win);
}
#endif /* EDITION */
