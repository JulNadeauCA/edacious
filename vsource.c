/*
 * Copyright (c) 2004-2008 Hypertriton, Inc. <http://hypertriton.com/>
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

/*
 * Independent voltage source class.
 */

#include <eda.h>
#include "vsource.h"

const ES_Port esVsourcePorts[] = {
	{ 0, "" },
	{ 1, "v+" },
	{ 2, "v-" },
	{ -1 },
};

/* Find the pairs connecting two contiguous ports, and its polarity. */
static int
FindContigPair(ES_Port *pA, ES_Port *pB, ES_Pair **Rdip, int *Rpol)
{
	ES_Component *com = pA->com;
	ES_Circuit *ckt = com->ckt;
	ES_Pair *dip;
	Uint i;

	COMPONENT_FOREACH_PAIR(dip, i, com) {
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

		NODE_FOREACH_BRANCH(br, node) {
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
InsertVoltageLoop(ES_Vsource *vs)
{
	ES_Loop *lnew;
	Uint i;

	/* Create a new voltage source loop entry. */
	lnew = Malloc(sizeof(ES_Loop));
	lnew->name = 1+vs->nloops++;
	lnew->pairs = Malloc((vs->nlstack-1)*sizeof(ES_Pair *));
	lnew->npairs = 0;
	lnew->origin = vs;

	for (i = 1; i < vs->nlstack; i++) {
		ES_Pair *dip = NULL;
		int pol = 0;

		if (!FindContigPair(vs->lstack[i], vs->lstack[i-1], &dip, &pol))
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
FindLoops(ES_Vsource *vs, ES_Port *portCur)
{
	ES_Circuit *ckt = COM(vs)->ckt;
	ES_Node *nodeCur = ckt->nodes[portCur->node], *nodeNext;
	ES_Port *portNext;
	ES_Branch *br;
	Uint i;

	nodeCur->flags |= CKTNODE_EXAM;

	vs->lstack = Realloc(vs->lstack, (vs->nlstack+1)*sizeof(ES_Port *));
	vs->lstack[vs->nlstack++] = portCur;

	NODE_FOREACH_BRANCH(br, nodeCur) {
		if (br->port == portCur) {
			continue;
		}
		if (br->port->com == COM(vs) &&
		    br->port->n == 2 &&
		    vs->nlstack > 2) {
			vs->lstack = Realloc(vs->lstack,
			    (vs->nlstack+1)*sizeof(ES_Port *));
			vs->lstack[vs->nlstack++] = &COM(vs)->ports[2];
			InsertVoltageLoop(vs);
			vs->nlstack--;
			continue;
		}
		if (br->port->com == NULL) {
			continue;
		}
		COMPONENT_FOREACH_PORT(portNext, i, br->port->com) {
			nodeNext = ckt->nodes[portNext->node];
			if ((portNext == portCur) ||
			    (portNext->com == br->port->com &&
			     portNext->n == br->port->n) ||
			    (nodeNext->flags & CKTNODE_EXAM)) {
				continue;
			}
			FindLoops(vs, portNext);
		}
	}

	vs->nlstack--;
	nodeCur->flags &= ~(CKTNODE_EXAM);
}

/* Isolate the loops relative to the given voltage source. */
void
ES_VsourceFindLoops(ES_Vsource *vs)
{
	Uint i;

	vs->lstack = Malloc(sizeof(ES_Port *));
	FindLoops(vs, PORT(vs,1));

	Free(vs->lstack);
	vs->lstack = NULL;
	vs->nlstack = 0;
}

/*
 * Ideal voltage sources require the addition of one unknown (iE), and a
 * branch current equation (br). Voltage sources contribute two entries to
 * the B/C matrices and one entry to the RHS vector.
 *
 *    |  Vk  Vj  iE  | RHS
 *    |--------------|-----
 *  k |           1  |
 *  j |          -1  |
 * br | 1   -1       | Ekj
 *    |--------------|-----
 */
static int
LoadDC_BCD(void *p, M_Matrix *B, M_Matrix *C, M_Matrix *D)
{
	ES_Vsource *vs = p;
	Uint k = PNODE(vs,1);
	Uint j = PNODE(vs,2);
	int v;

	if ((v = ES_VsourceName(vs)) == -1) {
		AG_SetError("no such vsource");
		return (-1);
	}
	if (k != 0) {
		B->v[k][v] = 1.0;
		C->v[v][k] = 1.0;
	}
	if (j != 0) {
		B->v[j][v] = -1.0;
		C->v[v][j] = -1.0;
	}
	return (0);
}
static int
LoadDC_RHS(void *p, M_Vector *i, M_Vector *e)
{
	ES_Vsource *vs = p;
	int v;

	if ((v = ES_VsourceName(vs)) == -1 ) {
		AG_SetError("no such vsource");
		return (-1);
	}
	e->v[v][0] = vs->voltage;
	return (0);
}

static void
Connected(AG_Event *event)
{
	ES_Circuit *ckt = AG_SENDER();
	ES_Vsource *vs = AG_SELF();

	ckt->vsrcs = Realloc(ckt->vsrcs, (ckt->m+1)*sizeof(ES_Vsource *));
	ckt->vsrcs[ckt->m] = vs;
	Debug(ckt, "Added voltage source #%d\n", ckt->m);
	ckt->m++;
}

static void
Disconnected(AG_Event *event)
{
	ES_Circuit *ckt = AG_SENDER();
	ES_Vsource *vs = AG_SELF();
	Uint i, j;
	
	for (i = 0; i < ckt->m; i++) {
		if (ckt->vsrcs[i] == vs) {
			if (i < --ckt->m) {
				for (; i < ckt->m; i++)
					ckt->vsrcs[i] = ckt->vsrcs[i+1];
			}
			Debug(ckt, "Removed voltage source #%d\n", i);
			break;
		}
	}
}

static void
Init(void *p)
{
	ES_Vsource *vs = p;

	ES_InitPorts(vs, esVsourcePorts);
	vs->voltage = 5;
	vs->lstack = NULL;
	vs->nlstack = 0;
	vs->nloops = 0;
	TAILQ_INIT(&vs->loops);

	COM(vs)->loadDC_BCD = LoadDC_BCD;
	COM(vs)->loadDC_RHS = LoadDC_RHS;
	AG_SetEvent(vs, "circuit-connected", Connected, NULL);
	AG_SetEvent(vs, "circuit-disconnected", Disconnected, NULL);
}

static void
FreeDataset(void *p)
{
	ES_Vsource *vs = p;
	ES_VsourceFreeLoops(vs);
}

void
ES_VsourceFreeLoops(ES_Vsource *vs)
{
	ES_Loop *loop, *nloop;
	Uint i;

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

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Vsource *vs = p;

	vs->voltage = AG_ReadDouble(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Vsource *vs = p;

	AG_WriteDouble(buf, vs->voltage);
	return (0);
}

static int
Export(void *p, enum circuit_format fmt, FILE *f)
{
	ES_Vsource *vs = p;

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

	for (i = 0; i < ckt->m; i++) {
		if (VSOURCE(ckt->vsrcs[i]) == vs)
			return (i);
	}
	return (-1);
}

static void
PollLoops(AG_Event *event)
{
	char text[AG_TLIST_LABEL_MAX];
	ES_Vsource *vs = AG_PTR(1);
	AG_Tlist *tl = AG_SELF();
	AG_TlistItem *it;
	ES_Loop *l;
	Uint i, j;
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

			Snprintf(text, sizeof(text), "%s:(%s,%s)",
			    OBJECT(dip->com)->name, dip->p1->name,
			    dip->p2->name);
			it = AG_TlistAddPtr(tl, NULL, text,
			    dip);
			it->depth = 1;
		}
	}
	AG_TlistRestore(tl);
}

static void *
Edit(void *p)
{
	ES_Vsource *vs = p;
	AG_Window *win;
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

ES_ComponentClass esVsourceClass = {
	{
		"ES_Component:ES_Vsource",
		sizeof(ES_Vsource),
		{ 0,0 },
		Init,
		FreeDataset,
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Independent voltage source"),
	"V",
	"Sources/Vsource.eschem",
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	Export,
	NULL			/* connect */
};
