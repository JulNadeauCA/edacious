/*
 * Copyright (c) 2006-2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Electrical circuit.
 */

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>
#include <agar/sc.h>
#include <agar/dev.h>

#include "eda.h"
#include "spice.h"
#include "scope.h"

static void InitNode(ES_Node *, Uint);
static void InitGround(ES_Circuit *);

void
ES_ResumeSimulation(ES_Circuit *ckt)
{
	if (ckt->sim != NULL && ckt->sim->ops->start != NULL)
		ckt->sim->ops->start(ckt->sim);
}

void
ES_SuspendSimulation(ES_Circuit *ckt)
{
	if (ckt->sim != NULL && ckt->sim->ops->stop != NULL)
		ckt->sim->ops->stop(ckt->sim);
}

#if 0
static void
Detached(AG_Event *event)
{
	ES_Circuit *ckt = AG_SELF();

	ES_DestroySimulation(ckt);
}
#endif

static void
EditOpen(AG_Event *event)
{
	ES_Circuit *ckt = AG_SELF();
	ES_Component *com;

	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		AG_PostEvent(ckt, com, "circuit-shown", NULL);
		AG_ObjectPageIn(com);
	}
}

static void
EditClose(AG_Event *event)
{
	ES_Circuit *ckt = AG_SELF();
	ES_Component *com;

	ES_DestroySimulation(ckt);

	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		AG_PostEvent(ckt, com, "circuit-hidden", NULL);
		AG_ObjectPageOut(com);
	}
}

static SC_Real
ReadNodeVoltage(void *p, AG_Prop *pr)
{
	return (ES_NodeVoltage(p, atoi(&pr->key[1])));
}

static SC_Real
ReadBranchCurrent(void *p, AG_Prop *pr)
{
	return (ES_BranchCurrent(p, atoi(&pr->key[1])));
}

/* Effect a change in the circuit topology.  */
void
ES_CircuitModified(ES_Circuit *ckt)
{
	char key[32];
	ES_Component *com;
	ES_Vsource *vs;
	ES_Loop *loop;
	Uint i;
	AG_Prop *pr;

	/* Regenerate loop and pair information. */
	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		if (com->flags & COMPONENT_FLOATING) {
			continue;
		}
		for (i = 0; i < com->npairs; i++)
			com->pairs[i].nloops = 0;
	}
	AGOBJECT_FOREACH_CLASS(vs, ckt, es_vsource, "ES_Component:"
	                                            "ES_Vsource:*") {
		if (COM(vs)->flags & COMPONENT_FLOATING ||
		    PORT(vs,1)->node == -1 ||
		    PORT(vs,2)->node == -1) {
			continue;
		}
		ES_VsourceFreeLoops(vs);
		ES_VsourceFindLoops(vs);
	}
#if 0
	AGOBJECT_FOREACH_CHILD(com, ckt, es_component)
		AG_PostEvent(ckt, com, "circuit-modified", NULL);
#endif
	if (ckt->loops == NULL) {
		ckt->loops = Malloc(sizeof(ES_Loop *));
	}
	ckt->l = 0;
	AGOBJECT_FOREACH_CLASS(vs, ckt, es_vsource, "ES_Component:"
	                                            "ES_Vsource:*") {
		if (COM(vs)->flags & COMPONENT_FLOATING) {
			continue;
		}
		TAILQ_FOREACH(loop, &vs->loops, loops) {
			ckt->loops = Realloc(ckt->loops,
			    (ckt->l+1)*sizeof(ES_Loop *));
			ckt->loops[ckt->l++] = loop;
			loop->name = ckt->l;
		}
	}

	if (ckt->sim != NULL &&
	    ckt->sim->ops->cktmod != NULL)
		ckt->sim->ops->cktmod(ckt->sim, ckt);
#if 1
	AG_ObjectFreeProps(AGOBJECT(ckt));
	for (i = 1; i <= ckt->n; i++) {
		snprintf(key, sizeof(key), "v%u", i);
		pr = SC_SetReal(ckt, key, 0.0);
		SC_SetRealRdFn(pr, ReadNodeVoltage);
	}
	for (i = 1; i <= ckt->m; i++) {
		snprintf(key, sizeof(key), "I%u", i);
		pr = SC_SetReal(ckt, key, 0.0);
		SC_SetRealRdFn(pr, ReadBranchCurrent);
	}
#endif
}

static void
PostSimulationStep(AG_Event *event)
{
	ES_Circuit *ckt = AG_SELF();
	AG_Object *obj;

	AGOBJECT_FOREACH_CHILD(obj, ckt, ag_object)
		AG_PostEvent(ckt, obj, "circuit-step-end", NULL);
}

static void
Init(void *p)
{
	ES_Circuit *ckt = p;
	AG_Event *ev;
	VG_Style *vgs;
	VG *vg;

	ckt->flags = CIRCUIT_SHOW_NODES|CIRCUIT_SHOW_NODESYMS;
	ckt->simlock = 0;
	ckt->descr[0] = '\0';
	ckt->sim = NULL;
	ckt->loops = NULL;
	ckt->vsrcs = NULL;
	ckt->m = 0;
	ckt->nodes = Malloc(sizeof(ES_Node *));
	ckt->n = 0;
	ckt->console = NULL;
	AG_MutexInitRecursive(&ckt->lock);
	TAILQ_INIT(&ckt->wires);
	TAILQ_INIT(&ckt->syms);
	
	InitGround(ckt);

	vg = ckt->vg = VG_New(VG_VISGRID);
	Strlcpy(vg->layers[0].name, _("Schematic"), sizeof(vg->layers[0].name));
	VG_SetBackgroundColor(vg, VG_GetColorRGB(0,0,0));
	VG_SetGridColor(vg, VG_GetColorRGB(120,120,120));

	ckt->vgblk = VG_BeginBlock(vg, ".generic", 0);
	VG_EndBlock(vg);

	vgs = VG_CreateStyle(vg, VG_TEXT_STYLE, "component-name");
	vgs->vg_text_st.size = 14;
	vgs = VG_CreateStyle(vg, VG_TEXT_STYLE, "node-name");
	vgs->vg_text_st.size = 12;
	
	AG_SetEvent(ckt, "edit-open", EditOpen, NULL);
	AG_SetEvent(ckt, "edit-close", EditClose, NULL);

	/* Notify visualization objects of simulation progress. */
	ev = AG_SetEvent(ckt, "circuit-step-begin", NULL, NULL);
	ev->flags |= AG_EVENT_PROPAGATE;
	ev = AG_SetEvent(ckt, "circuit-step-end", PostSimulationStep, NULL);
	ev->flags |= AG_EVENT_PROPAGATE;
}

static void
FreeDataset(void *p)
{
	ES_Circuit *ckt = p;
	ES_Component *com;
	ES_Wire *wire, *nwire;
	ES_Sym *sym, *nsym;
	Uint i;

	ES_DestroySimulation(ckt);

	Free(ckt->loops);
	ckt->loops = NULL;
	ckt->l = 0;

	Free(ckt->vsrcs);
	ckt->vsrcs = NULL;
	ckt->m = 0;

	for (wire = TAILQ_FIRST(&ckt->wires);
	     wire != TAILQ_END(&ckt->wires);
	     wire = nwire) {
		nwire = TAILQ_NEXT(wire, wires);
		Free(wire);
	}
	TAILQ_INIT(&ckt->wires);
	
	for (sym = TAILQ_FIRST(&ckt->syms);
	     sym != TAILQ_END(&ckt->syms);
	     sym = nsym) {
		nsym = TAILQ_NEXT(sym, syms);
		Free(sym);
	}
	TAILQ_INIT(&ckt->syms);

	for (i = 0; i <= ckt->n; i++) {
		ES_CircuitFreeNode(ckt->nodes[i]);
		Free(ckt->nodes[i]);
	}
	ckt->nodes = Realloc(ckt->nodes, sizeof(ES_Node *));
	ckt->n = 0;
	InitGround(ckt);

	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		if (com->flags & COMPONENT_FLOATING) {
			continue;
		}
		for (i = 1; i <= com->nports; i++) {
			ES_Port *port = &com->ports[i];

			port->node = -1;
			port->branch = NULL;
			port->selected = 0;
		}
		com->block = NULL;
	}
	VG_Reinit(ckt->vg);
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Circuit *ckt = p;
	ES_Component *com;
	Uint32 nitems;
	Uint i, j, nnodes;

	AG_CopyString(ckt->descr, buf, sizeof(ckt->descr));
	ckt->flags = AG_ReadUint32(buf);

	/* Load the circuit nodes. */
	nnodes = (Uint)AG_ReadUint32(buf);
	for (i = 0; i <= nnodes; i++) {
		int nbranches, flags;
		int name;

		flags = (int)AG_ReadUint32(buf);
		nbranches = (int)AG_ReadUint32(buf);
		if (i == 0) {
			name = 0;
			ES_CircuitFreeNode(ckt->nodes[0]);
			Free(ckt->nodes[0]);
			InitGround(ckt);
		} else {
			name = ES_CircuitAddNode(ckt, flags & ~(CKTNODE_EXAM));
		}

		for (j = 0; j < nbranches; j++) {
			char ppath[AG_OBJECT_PATH_MAX];
			ES_Component *pcom;
			ES_Branch *br;
			int pport;

			AG_CopyString(ppath, buf, sizeof(ppath));
			AG_ReadUint32(buf);			/* Pad */
			pport = (int)AG_ReadUint32(buf);
			if ((pcom = AG_ObjectFind(ckt, ppath)) == NULL) {
				AG_SetError("%s", AG_GetError());
				return (-1);
			}
			if (pport > pcom->nports) {
				AG_SetError("%s: port #%d > %d", ppath, pport,
				    pcom->nports);
				return (-1);
			}
			br = ES_CircuitAddBranch(ckt, name,
			    &pcom->ports[pport]);
			pcom->ports[pport].node = name;
			pcom->ports[pport].branch = br;
		}
	}

	/* Load the schematics. */
	if (VG_Load(ckt->vg, buf) == -1) {
		return (-1);
	}
	if ((ckt->vgblk = VG_GetBlock(ckt->vg, ".generic")) == NULL) {
		AG_SetError("Missing .generic block");
		return (-1);
	}

	/* Load the schematic wires. */
	nitems = AG_ReadUint32(buf);
	for (i = 0; i < nitems; i++) {
		ES_Wire *wire;
		
		wire = ES_CircuitAddWire(ckt);
		wire->flags = (Uint)AG_ReadUint32(buf);
		wire->cat = (Uint)AG_ReadUint32(buf);
		for (i = 0; i < 2; i++) {
			ES_Port *port = &wire->ports[i];

			port->x = AG_ReadFloat(buf);
			port->y = AG_ReadFloat(buf);
			port->node = (int)AG_ReadUint32(buf);
		}
	}

	/* Load the symbol table. */
	nitems = AG_ReadUint32(buf);
	for (i = 0; i < nitems; i++) {
		char name[CIRCUIT_SYM_MAX];
		ES_Sym *sym;
		
		sym = ES_CircuitAddSym(ckt);
		AG_CopyString(sym->name, buf, sizeof(sym->name));
		AG_CopyString(sym->descr, buf, sizeof(sym->descr));
		sym->type = (enum es_sym_type)AG_ReadUint32(buf);
		AG_ReadUint32(buf);				/* Pad */
		switch (sym->type) {
		case ES_SYM_NODE:
			sym->p.node = (int)AG_ReadSint32(buf);
			break;
		case ES_SYM_VSOURCE:
			sym->p.vsource = (int)AG_ReadSint32(buf);
			break;
		case ES_SYM_ISOURCE:
			sym->p.isource = (int)AG_ReadSint32(buf);
			break;
		}
	}

	/* Load the component port assignments. */
	nitems = AG_ReadUint32(buf);
	for (i = 0; i < nitems; i++) {
		char name[AG_OBJECT_NAME_MAX];
		char path[AG_OBJECT_PATH_MAX];
		int j, nports;
		ES_Component *com;

		/* Lookup the component. */
		AG_CopyString(name, buf, sizeof(name));
		AGOBJECT_FOREACH_CLASS(com, ckt, es_component,
		    "ES_Component:*") {
			if (strcmp(AGOBJECT(com)->name, name) == 0)
				break;
		}
		if (com == NULL) { Fatal("unexisting component"); }
		
		/* Reassign the block pointer since the vg was reloaded. */
		AG_ObjectCopyName(com, path, sizeof(path));
		if ((com->block = VG_GetBlock(ckt->vg, path)) == NULL)
			Fatal("unexisting block");

		/* Load the port information. */
		com->nports = (int)AG_ReadUint32(buf);
		for (j = 1; j <= com->nports; j++) {
			ES_Port *port = &com->ports[j];
			VG_Block *block_save;
			ES_Branch *br;
			ES_Node *node;

			port->n = (int)AG_ReadUint32(buf);
			AG_CopyString(port->name, buf, sizeof(port->name));
			port->x = AG_ReadFloat(buf);
			port->y = AG_ReadFloat(buf);
			port->node = (int)AG_ReadUint32(buf);
			node = ckt->nodes[port->node];

			TAILQ_FOREACH(br, &node->branches, branches) {
				if (br->port->com == com &&
				    br->port->n == port->n)
					break;
			}
			if (br == NULL) { Fatal("Illegal branch"); }
			port->branch = br;
			port->selected = 0;
		}
	}

	/* Send circuit-connected event to components. */
	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		AG_PostEvent(ckt, com, "circuit-connected", NULL);
	}

	ES_CircuitModified(ckt);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	char path[AG_OBJECT_PATH_MAX];
	ES_Circuit *ckt = p;
	ES_Node *node;
	ES_Branch *br;
	ES_Component *com;
	ES_Wire *wire;
	ES_Sym *sym;
	off_t count_offs;
	Uint32 count;
	Uint i;

	AG_WriteString(buf, ckt->descr);
	AG_WriteUint32(buf, ckt->flags);
	AG_WriteUint32(buf, ckt->n);
	for (i = 0; i <= ckt->n; i++) {
		ES_Node *node = ckt->nodes[i];

		AG_WriteUint32(buf, (Uint32)node->flags);
		count_offs = AG_Tell(buf);
		count = 0;
		AG_WriteUint32(buf, 0);
		TAILQ_FOREACH(br, &node->branches, branches) {
			if (br->port == NULL || br->port->com == NULL ||
			    br->port->com->flags & COMPONENT_FLOATING) {
				continue;
			}
			AG_ObjectCopyName(br->port->com, path, sizeof(path));
			AG_WriteString(buf, path);
			AG_WriteUint32(buf, 0);			/* Pad */
			AG_WriteUint32(buf, (Uint32)br->port->n);
			count++;
		}
		AG_WriteUint32At(buf, count, count_offs);
	}
	
	/* Save the schematics. */
	VG_Save(ckt->vg, buf);

	/* Save the schematic wires. */
	count_offs = AG_Tell(buf);
	count = 0;
	AG_WriteUint32(buf, 0);
	TAILQ_FOREACH(wire, &ckt->wires, wires) {
		AG_WriteUint32(buf, (Uint32)wire->flags);
		AG_WriteUint32(buf, (Uint32)wire->cat);
		for (i = 0; i < 2; i++) {
			ES_Port *port = &wire->ports[i];

			AG_WriteFloat(buf, port->x);
			AG_WriteFloat(buf, port->y);
			AG_WriteUint32(buf, (Uint32)port->node);
#ifdef DEBUG
			if (port->node == -1) { Fatal("Illegal wire port"); }
#endif
		}
		count++;
	}
	AG_WriteUint32At(buf, count, count_offs);

	/* Save the symbol table. */
	count_offs = AG_Tell(buf);
	count = 0;
	AG_WriteUint32(buf, 0);
	TAILQ_FOREACH(sym, &ckt->syms, syms) {
		AG_WriteString(buf, sym->name);
		AG_WriteString(buf, sym->descr);
		AG_WriteUint32(buf, (Uint32)sym->type);
		AG_WriteUint32(buf, 0);				/* Pad */
		switch (sym->type) {
		case ES_SYM_NODE:
			AG_WriteSint32(buf, (Sint32)sym->p.node);
			break;
		case ES_SYM_VSOURCE:
			AG_WriteSint32(buf, (Sint32)sym->p.vsource);
			break;
		case ES_SYM_ISOURCE:
			AG_WriteSint32(buf, (Sint32)sym->p.isource);
			break;
		}
		count++;
	}
	AG_WriteUint32At(buf, count, count_offs);

	/* Save the component information. */
	count_offs = AG_Tell(buf);
	count = 0;
	AG_WriteUint32(buf, 0);
	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		if (com->flags & COMPONENT_FLOATING) {
			continue;
		}
		AG_WriteString(buf, AGOBJECT(com)->name);
		AG_WriteUint32(buf, (Uint32)com->nports);
		for (i = 1; i <= com->nports; i++) {
			ES_Port *port = &com->ports[i];

			AG_WriteUint32(buf, (Uint32)port->n);
			AG_WriteString(buf, port->name);
			AG_WriteFloat(buf, port->x);
			AG_WriteFloat(buf, port->y);
			AG_WriteUint32(buf, (Uint32)port->node);
#ifdef DEBUG
			if (port->node == -1) { Fatal("Illegal com port"); }
#endif
		}
		count++;
	}
	AG_WriteUint32At(buf, count, count_offs);
	return (0);
}

static void
InitNode(ES_Node *node, Uint flags)
{
	node->flags = flags;
	node->nbranches = 0;
	TAILQ_INIT(&node->branches);
}

static void
InitGround(ES_Circuit *ckt)
{
	ckt->nodes[0] = Malloc(sizeof(ES_Node));
	InitNode(ckt->nodes[0], CKTNODE_REFERENCE);
	ES_CircuitAddNodeSym(ckt, "Gnd", 0);
}

/* Create a new node with no branches. */
int
ES_CircuitAddNode(ES_Circuit *ckt, Uint flags)
{
	ES_Node *node;
	int n = ++ckt->n;

	ckt->nodes = Realloc(ckt->nodes, (n+1)*sizeof(ES_Node *));
	ckt->nodes[n] = Malloc(sizeof(ES_Node));
	InitNode(ckt->nodes[n], flags);
	return (n);
}

ES_Wire *
ES_CircuitAddWire(ES_Circuit *ckt)
{
	ES_Wire *wire;
	int i;

	wire = Malloc(sizeof(ES_Wire));
	wire->flags = 0;
	wire->cat = 0;
	for (i = 0; i < 2; i++) {
		ES_Port *port = &wire->ports[i];

		port->n = i+1;
		port->name[0] = '\0';
		port->x = 0.0;
		port->y = 0.0;
		port->com = NULL;
		port->node = -1;
		port->branch = NULL;
		port->selected = 0;
	}
	TAILQ_INSERT_TAIL(&ckt->wires, wire, wires);
	return (wire);
}

ES_Sym *
ES_CircuitAddNodeSym(ES_Circuit *ckt, const char *name, int node)
{
	ES_Sym *sym;

	sym = ES_CircuitAddSym(ckt);
	Strlcpy(sym->name, name, sizeof(sym->name));
	sym->type = ES_SYM_NODE;
	sym->p.node = node;
	return (sym);
}

ES_Sym *
ES_CircuitAddVsourceSym(ES_Circuit *ckt, const char *name, int vsource)
{
	ES_Sym *sym;

	sym = ES_CircuitAddSym(ckt);
	Strlcpy(sym->name, name, sizeof(sym->name));
	sym->type = ES_SYM_VSOURCE;
	sym->p.vsource = vsource;
	return (sym);
}

ES_Sym *
ES_CircuitAddIsourceSym(ES_Circuit *ckt, const char *name, int isource)
{
	ES_Sym *sym;

	sym = ES_CircuitAddSym(ckt);
	Strlcpy(sym->name, name, sizeof(sym->name));
	sym->type = ES_SYM_ISOURCE;
	sym->p.vsource = isource;
	return (sym);
}

ES_Sym *
ES_CircuitAddSym(ES_Circuit *ckt)
{
	ES_Sym *sym;

	sym = Malloc(sizeof(ES_Sym));
	sym->name[0] = '\0';
	sym->descr[0] = '\0';
	sym->type = 0;
	sym->p.node = 0;
	TAILQ_INSERT_TAIL(&ckt->syms, sym, syms);
	return (sym);
}

void
ES_CircuitDelSym(ES_Circuit *ckt, ES_Sym *sym)
{
	TAILQ_REMOVE(&ckt->syms, sym, syms);
	Free(sym);
}

ES_Sym *
ES_CircuitFindSym(ES_Circuit *ckt, const char *name)
{
	ES_Sym *sym;

	TAILQ_FOREACH(sym, &ckt->syms, syms) {
		if (strcmp(sym->name, name) == 0)
			break;
	}
	if (sym == NULL) {
		AG_SetError(_("%s: no such symbol: `%s'"),
		    AGOBJECT(ckt)->name, name);
	}
	return (sym);
}

void
ES_CircuitDelWire(ES_Circuit *ckt, ES_Wire *wire)
{
	TAILQ_REMOVE(&ckt->wires, wire, wires);
	Free(wire);
}

/*
 * Evaluate whether node n is connected to the voltage source m. If that
 * is the case, return the polarity.
 */
int
ES_NodeVsource(ES_Circuit *ckt, int n, int m, int *sign)
{
	ES_Node *node = ckt->nodes[n];
	ES_Branch *br;
	ES_Vsource *vs;

	TAILQ_FOREACH(br, &node->branches, branches) {
		if (br->port == NULL ||
		   (vs = VSOURCE(br->port->com)) == NULL) {
			continue;
		}
		if (COM(vs)->flags & COMPONENT_FLOATING ||
		   !AG_ObjectIsClass(vs, "ES_Component:ES_Vsource:*") ||
		   vs != ckt->vsrcs[m-1]) {
			continue;
		}
		if (br->port->n == 1) {
			*sign = +1;
		} else {
			*sign = -1;
		}
		return (1);
	}
	return (0);
}

/* Return the DC voltage for the node j from the last simulation step. */
SC_Real
ES_NodeVoltage(ES_Circuit *ckt, int j)
{
	if (ckt->sim == NULL || ckt->sim->ops->node_voltage == NULL) {
		ES_CircuitLog(ckt, _("Cannot get node %d voltage"), j);
		return (0.0);
	}
	return (ckt->sim->ops->node_voltage(ckt->sim, j));
}

/* Return the branch current for the voltage source k from the last step. */
SC_Real
ES_BranchCurrent(ES_Circuit *ckt, int k)
{
	if (ckt->sim == NULL || ckt->sim->ops->branch_current == NULL) {
		ES_CircuitLog(ckt, _("Cannot get branch %d current"), k);
		return (0.0);
	}
	return (ckt->sim->ops->branch_current(ckt->sim, k));
}

ES_Node *
ES_CircuitGetNode(ES_Circuit *ckt, int n)
{
	if (n < 0 || n > ckt->n) {
		Fatal("%s:%d: bad node (%d)", AGOBJECT(ckt)->name, n, ckt->n);
	}
	return (ckt->nodes[n]);
}

/* Return the matching symbol (or "nX") for a given node. */
void
ES_CircuitNodeSymbol(ES_Circuit *ckt, int n, char *dst, size_t dst_len)
{
	ES_Sym *sym;

	if (n < 0) {
		Strlcpy(dst, "(null)", sizeof(dst));
		return;
	}
	TAILQ_FOREACH(sym, &ckt->syms, syms) {
		if (sym->type == ES_SYM_NODE &&
		    sym->p.node == n) {
			Strlcpy(dst, sym->name, dst_len);
			return;
		}
	}
	snprintf(dst, dst_len, "n%d", n);
	return;
}

/* Search for a node matching the given name (symbol or "nX") */
ES_Node *
ES_CircuitFindNode(ES_Circuit *ckt, const char *name)
{
	ES_Sym *sym;

	TAILQ_FOREACH(sym, &ckt->syms, syms) {
		if (sym->type == ES_SYM_NODE &&
		    strcmp(sym->name, name) == 0 &&
		    sym->p.node >= 0 && sym->p.node <= ckt->n)
			return (ckt->nodes[sym->p.node]);
	}
	if (name[0] == 'n' && name[1] != '\0') {
		int n = atoi(&name[1]);

		if (n >= 0 && n <= ckt->n) {
			return (ckt->nodes[n]);
		}
	}
	AG_SetError(_("No such node: `%s'"), name);
	return (NULL);
}

void
ES_CircuitFreeNode(ES_Node *node)
{
	ES_Branch *br, *nbr;

	for (br = TAILQ_FIRST(&node->branches);
	     br != TAILQ_END(&node->branches);
	     br = nbr) {
		nbr = TAILQ_NEXT(br, branches);
		Free(br);
	}
	TAILQ_INIT(&node->branches);
}

/*
 * Remove a node and all references to it. If necessary, shift the entire
 * node array and update all port references accordingly.
 */
void
ES_CircuitDelNode(ES_Circuit *ckt, int n)
{
	ES_Node *node;
	ES_Branch *br;
	int i;

#ifdef DEBUG
	if (n == 0 || n > ckt->n) { Fatal("Illegal node"); }
#endif
	node = ckt->nodes[n];
	ES_CircuitFreeNode(node);
	Free(node);

	if (n != ckt->n) {
		for (i = n; i <= ckt->n; i++) {
			TAILQ_FOREACH(br, &ckt->nodes[i]->branches, branches) {
				if (br->port != NULL && br->port->com != NULL)
					br->port->node = i-1;
			}
		}
		memmove(&ckt->nodes[n], &ckt->nodes[n+1],
		    (ckt->n - n) * sizeof(ES_Node *));
	}

	ckt->n--;
	ES_CircuitLog(ckt, _("Removed node %d"), n);
}

int
ES_CircuitNodeNameByPtr(ES_Circuit *ckt, ES_Node *np) 
{
	int i;

	for (i = 1; i <= ckt->n; i++) {
		if (ckt->nodes[i] == np)
			return (i);
	}
	AG_SetError(_("No such node"));
	return (-1);
}

int
ES_CircuitDelNodeByPtr(ES_Circuit *ckt, ES_Node *np)
{
	int i;

	for (i = 1; i <= ckt->n; i++) {
		if (ckt->nodes[i] == np) {
			ES_CircuitDelNode(ckt, i);
			return (0);
		}
	}
	AG_SetError(_("No such node"));
	return (-1);
}

/* Merge the branches of two nodes, creating a new node from them. */
int
ES_CircuitMergeNodes(ES_Circuit *ckt, int N1, int N2)
{
	ES_Branch *br;
	ES_Node *n1 = ckt->nodes[N1];
	ES_Node *n2 = ckt->nodes[N2];
	ES_Node *n3;
	int N3;

	if (N1 == 0) {
		TAILQ_FOREACH(br, &n2->branches, branches) {
			br->port->node = 0;
			ES_CircuitAddBranch(ckt, 0, br->port);
		}
		ES_CircuitFreeNode(n2);
		N3 = 0;
	} else if (N2 == 0) {
		TAILQ_FOREACH(br, &n1->branches, branches) {
			br->port->node = 0;
			ES_CircuitAddBranch(ckt, 0, br->port);
		}
		ES_CircuitFreeNode(n1);
		N3 = 0;
	} else {
		N3 = ES_CircuitAddNode(ckt,0);
		TAILQ_FOREACH(br, &n1->branches, branches) {
			br->port->node = N3;
			ES_CircuitAddBranch(ckt, N3, br->port);
		}
		TAILQ_FOREACH(br, &n2->branches, branches) {
			br->port->node = N3;
			ES_CircuitAddBranch(ckt, N3, br->port);
		}
		ES_CircuitFreeNode(n1);
		ES_CircuitFreeNode(n2);
	}

	ES_CircuitLog(ckt, _("Merged n%d,n%d into n%d"), N1, N2, N3);
	return (N3);
}

ES_Branch *
ES_CircuitAddBranch(ES_Circuit *ckt, int n, ES_Port *port)
{
	ES_Node *node = ckt->nodes[n];
	ES_Branch *br;

	br = Malloc(sizeof(ES_Branch));
	br->port = port;
	TAILQ_INSERT_TAIL(&node->branches, br, branches);
	node->nbranches++;
	return (br);
}

ES_Branch *
ES_CircuitGetBranch(ES_Circuit *ckt, int n, ES_Port *port)
{
	ES_Node *node = ckt->nodes[n];
	ES_Branch *br;

	TAILQ_FOREACH(br, &node->branches, branches) {
		if (br->port == port)
			break;
	}
	return (br);
}

void
ES_CircuitDelBranch(ES_Circuit *ckt, int n, ES_Branch *br)
{
	ES_Node *node = ckt->nodes[n];

	TAILQ_REMOVE(&node->branches, br, branches);
	Free(br);
#ifdef DEBUG
	if ((node->nbranches - 1) < 0) { Fatal("--nbranches < 0"); }
#endif
	node->nbranches--;
}

void
ES_CircuitFreeComponents(ES_Circuit *ckt)
{
	ES_Component *com, *ncom;

	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		AG_ObjectDetach(com);
		AG_ObjectDestroy(com);
	}
}

void
ES_DestroySimulation(ES_Circuit *ckt)
{
	if (ckt->sim != NULL) {
		ES_SuspendSimulation(ckt);
		ES_SimDestroy(ckt->sim);
		ckt->sim = NULL;
	}
}

static void
Destroy(void *p)
{
	ES_Circuit *ckt = p;
	
	VG_Destroy(ckt->vg);
	pthread_mutex_destroy(&ckt->lock);
}

/* Select the simulation mode. */
ES_Sim *
ES_SetSimulationMode(ES_Circuit *ckt, const ES_SimOps *sops)
{
	ES_Sim *sim;

	ES_DestroySimulation(ckt);

	ckt->sim = sim = Malloc(sops->size);
	sim->ckt = ckt;
	sim->ops = sops;
	sim->running = 0;

	if (sim->ops->init != NULL) {
		sim->ops->init(sim);
	}
	if (sim->ops->edit != NULL &&
	   (sim->win = sim->ops->edit(sim, ckt)) != NULL) {
		AG_WindowSetCaption(sim->win, "%s: %s", AGOBJECT(ckt)->name,
		    _(sim->ops->name));
		AG_WindowSetPosition(sim->win, AG_WINDOW_LOWER_LEFT, 0);
		AG_WindowShow(sim->win);
	}
	return (sim);
}

void
ES_CircuitLog(void *p, const char *fmt, ...)
{
	ES_Circuit *ckt = p;
	AG_ConsoleLine *ln;
	va_list args;

	if (ckt->console == NULL)
		return;

	ln = AG_ConsoleAppendLine(ckt->console, NULL);
	va_start(args, fmt);
	AG_Vasprintf(&ln->text, fmt, args);
	va_end(args);
	ln->len = strlen(ln->text);
}

/* Lock the circuit and suspend any continuous simulation in progress. */
void
ES_LockCircuit(ES_Circuit *ckt)
{
	AG_MutexLock(&ckt->lock);
	if (ckt->sim != NULL && ckt->sim->running) {
//		if (ckt->simlock++ == 0)
//			ES_SuspendSimulation(ckt);
	}
}

/* Resume any previously suspended simulation and unlock the circuit. */
void
ES_UnlockCircuit(ES_Circuit *ckt)
{
	if (ckt->sim != NULL && !ckt->sim->running) {
		if (--ckt->simlock == 0)
			ES_ResumeSimulation(ckt);
	}
	AG_MutexUnlock(&ckt->lock);
}

static void
PollCircuitLoops(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	int i;

	AG_TlistClear(tl);
	for (i = 0; i < ckt->l; i++) {
		char voltage[32];
		ES_Loop *loop = ckt->loops[i];
		ES_Vsource *vs = (ES_Vsource *)loop->origin;
		AG_TlistItem *it;

		AG_UnitFormat(vs->voltage, agEMFUnits, voltage,
		    sizeof(voltage));
		it = AG_TlistAdd(tl, NULL, "%s:L%u (%s)", AGOBJECT(vs)->name,
		    loop->name, voltage);
		it->p1 = loop;
	}
	AG_TlistRestore(tl);
}

static void
PollCircuitNodes(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	Uint i;

	AG_TlistClear(tl);
	for (i = 0; i <= ckt->n; i++) {
		ES_Node *node = ckt->nodes[i];
		ES_Branch *br;
		AG_TlistItem *it, *it2;

		it = AG_TlistAdd(tl, NULL, "n%u (0x%x, %d branches)", i,
		    node->flags, node->nbranches);
		it->p1 = node;
		it->depth = 0;
		TAILQ_FOREACH(br, &node->branches, branches) {
			if (br->port == NULL) {
				it = AG_TlistAdd(tl, NULL, "(null port)");
				it->p1 = br;
				it->depth = 1;
			} else {
				if (br->port->com != NULL) {
					it = AG_TlistAdd(tl, NULL, "%s:%s(%d)",
					    AGOBJECT(br->port->com)->name,
					    br->port->name, br->port->n);
					it->p1 = br;
					it->depth = 1;
				} else {
					it = AG_TlistAdd(tl, NULL, _("Wire"));
					it->p1 = br;
					it->depth = 1;
				}
			}
		}
	}
	AG_TlistRestore(tl);
}

static void
PollCircuitSources(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	int i;

	AG_TlistClear(tl);
	for (i = 0; i < ckt->m; i++) {
		ES_Vsource *vs = ckt->vsrcs[i];
		AG_TlistItem *it;

		it = AG_TlistAdd(tl, NULL, "%s", AGOBJECT(vs)->name);
		it->p1 = vs;
	}
	AG_TlistRestore(tl);
}

static void
SaveCircuit(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);

	if (AG_ObjectSave(ckt) == 0) {
		AG_TextTmsg(AG_MSG_INFO, 1250,
		    _("Circuit `%s' saved successfully."),
		    AGOBJECT(ckt)->name);
	} else {
		AG_TextMsg(AG_MSG_ERROR, "%s: %s", AGOBJECT(ckt)->name,
		    AG_GetError());
	}
}

static void
ShowTopology(AG_Event *event)
{
	AG_Window *pwin = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	AG_Window *win;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;
	AG_Tlist *tl;
	
	if ((win = AG_WindowNewNamed(0, "%s-topology", AGOBJECT(ckt)->name))
	    == NULL) {
		return;
	}
	AG_WindowSetCaption(win, _("%s: Circuit topology"),
	    AGOBJECT(ckt)->name);
	
	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);
	ntab = AG_NotebookAddTab(nb, _("Nodes"), AG_BOX_VERT);
	{
		tl = AG_TlistNew(ntab, AG_TLIST_POLL|AG_TLIST_TREE|
		                       AG_TLIST_EXPAND);
		AG_SetEvent(tl, "tlist-poll", PollCircuitNodes, "%p", ckt);
	}
	
	ntab = AG_NotebookAddTab(nb, _("Loops"), AG_BOX_VERT);
	{
		tl = AG_TlistNew(ntab, AG_TLIST_POLL|AG_TLIST_EXPAND);
		AG_SetEvent(tl, "tlist-poll", PollCircuitLoops, "%p", ckt);
	}
	
	ntab = AG_NotebookAddTab(nb, _("Sources"), AG_BOX_VERT);
	{
		tl = AG_TlistNew(ntab, AG_TLIST_POLL|AG_TLIST_EXPAND);
		AG_SetEvent(tl, "tlist-poll", PollCircuitSources, "%p", ckt);
	}
	
	AG_WindowAttach(pwin, win);
	AG_WindowSetGeometryAligned(win, AG_WINDOW_TR, 200, 300);
	AG_WindowShow(win);
}

static void
ShowLayoutSettings(AG_Event *event)
{
	char path[AG_OBJECT_PATH_MAX];
	AG_Window *pwin = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	VG_View *vv = AG_PTR(3);
	AG_Window *win;
	AG_MFSpinbutton *mfsu;
	AG_Numerical *num;
	AG_Textbox *tb;
	AG_Checkbox *cb;
	
	AG_ObjectCopyName(ckt, path, sizeof(path));
	if ((win = AG_WindowNewNamed(0, "settings-%s", path)) == NULL) {
		return;
	}
	AG_WindowSetCaption(win, _("Circuit layout: %s"), AGOBJECT(ckt)->name);
	AG_WindowSetPosition(win, AG_WINDOW_UPPER_RIGHT, 0);

	tb = AG_TextboxNew(win, AG_TEXTBOX_HFILL, _("Description: "));
	AG_TextboxBindUTF8(tb, ckt->descr, sizeof(ckt->descr));

	num = AG_NumericalNewFlt(win, 0, NULL, _("Grid spacing: "),
	    &vv->gridIval);
	AG_NumericalSetMin(num, 0.0625);
	AG_NumericalSetIncrement(num, 0.0625);
#if 0
	AG_LabelNew(win, AG_LABEL_STATIC, _("Nodes:"));
	tl = AG_TlistNew(win, AG_TLIST_POLL|AG_TLIST_TREE);
	AGWIDGET(tl)->flags &= ~(AG_WIDGET_HFILL);
	AG_SetEvent(tl, "tlist-poll", poll_nodes, "%p", ckt);
#endif	

	AG_WindowAttach(pwin, win);
	AG_WindowShow(win);
}

static void
ExportToSPICE(AG_Event *event)
{
	char name[FILENAME_MAX];
	ES_Circuit *ckt = AG_PTR(1);

	Strlcpy(name, AGOBJECT(ckt)->name, sizeof(name));
	Strlcat(name, ".cir", sizeof(name));

	if (ES_CircuitExportSPICE3(ckt, name) == -1)
		AG_TextMsg(AG_MSG_ERROR, "%s: %s", AGOBJECT(ckt)->name,
		    AG_GetError());
}

static void
CircuitViewButtondown(AG_Event *event)
{
	VG_View *vv =  AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	int button = AG_INT(2);
	float x = AG_FLOAT(3);
	float y = AG_FLOAT(4);
	ES_Component *com;
	VG_Block *block;

	if (button != SDL_BUTTON_RIGHT) {
		return;
	}
	block = VG_BlockClosest(ckt->vg, x, y);

	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		if (AGOBJECT(com)->cls->edit == NULL ||
		    com->block != block) {
			continue;
		}
		ES_ComponentOpenMenu(com, vv);
		break;
	}
}

static void
CircuitViewKeydown(AG_Event *event)
{
	VG_View *vv =  AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
}

static void
CircuitDoubleClick(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	int button = AG_INT(2);
	float x = AG_INT(3);
	float y = AG_INT(4);
	ES_Component *com;
	VG_Block *block;

	if (button != SDL_BUTTON_LEFT) {
		return;
	}
	block = VG_BlockClosest(ckt->vg, x, y);

	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		if (AGOBJECT(com)->cls->edit == NULL ||
		    com->block != block) {
			continue;
		}
		DEV_BrowserOpenData(com);
		break;
	}
}

static void
CircuitSelectSim(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	ES_SimOps *sops = AG_PTR(2);
	AG_Window *pwin = AG_PTR(3);
	ES_Sim *sim;

	if (ckt->sim != NULL && ckt->sim->running) {
		if (ckt->sim->win != NULL) {
			AG_WindowShow(ckt->sim->win);
		}
		AG_TextMsg(AG_MSG_ERROR, _("%s: simulation in progress"),
		    AGOBJECT(ckt)->name);
		return;
	}

	sim = ES_SetSimulationMode(ckt, sops);
	if (sim->win != NULL)
		AG_WindowAttach(pwin, sim->win);
}

static void
CircuitSelectTool(AG_Event *event)
{
	VG_View *vv = AG_PTR(1);
	VG_Tool *tool = AG_PTR(2);
	ES_Circuit *ckt = AG_PTR(3);

	VG_ViewSelectTool(vv, tool, ckt);
}

static void
FindCircuitObjs(AG_Tlist *tl, AG_Object *pob, int depth, void *ckt)
{
	AG_Object *cob;
	AG_TlistItem *it;
	SDL_Surface *icon;

	it = AG_TlistAdd(tl, NULL, "%s%s", pob->name,
	    (pob->flags & AG_OBJECT_RESIDENT) ? " (resident)" : "");
	it->depth = depth;
	it->cat = "object";
	it->p1 = pob;

	if (!TAILQ_EMPTY(&pob->children)) {
		it->flags |= AG_TLIST_HAS_CHILDREN;
		if (pob == AGOBJECT(ckt))
			it->flags |= AG_TLIST_VISIBLE_CHILDREN;
	}
	if ((it->flags & AG_TLIST_HAS_CHILDREN) &&
	    AG_TlistVisibleChildren(tl, it)) {
		TAILQ_FOREACH(cob, &pob->children, cobjs)
			FindCircuitObjs(tl, cob, depth+1, ckt);
	}
}

static void
PollCircuitObjs(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	AG_Object *ckt = AG_PTR(1);
	AG_TlistItem *it;

	AG_TlistClear(tl);
	AG_LockVFS(ckt);
	FindCircuitObjs(tl, ckt, 0, ckt);
	AG_UnlockVFS(ckt);
	AG_TlistRestore(tl);
}

static void
ShowConsole(AG_Event *event)
{
	AG_Window *pwin = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	AG_Window *win;

	if ((win = AG_WindowNewNamed(0, "%s-console", AGOBJECT(ckt)->name))
	    == NULL) {
		return;
	}
	AG_WindowSetCaption(win, _("Console: %s"), AGOBJECT(ckt)->name);
	ckt->console = AG_ConsoleNew(win, AG_CONSOLE_EXPAND);
	AG_WidgetFocus(ckt->console);
	AG_WindowAttach(pwin, win);

	AG_WindowSetGeometryAligned(win, AG_WINDOW_MC, agView->w/2, 100);
	AG_WindowShow(win);
}

static void
CircuitCreateView(AG_Event *event)
{
	AG_Window *pwin = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	VG_View *vv;
	AG_Window *win;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("View of %s"), AGOBJECT(ckt)->name);

	vv = VG_ViewNew(win, ckt->vg, VG_VIEW_EXPAND);
	VG_ViewSetScale(vv, 16.0f);
	AG_WidgetFocus(vv);

	AG_WindowSetGeometryAligned(win, AG_WINDOW_TR, 320, 240);
	AG_WindowAttach(pwin, win);
	AG_WindowShow(win);
}

static void
PollComponentPorts(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	ES_Component *com;
	int i;
	
	AG_TlistClear(tl);
	
	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		if (com->selected)
			break;
	}
	if (com == NULL)
		goto out;

	pthread_mutex_lock(&com->lock);
	for (i = 1; i <= com->nports; i++) {
		char text[AG_TLIST_LABEL_MAX];
		ES_Port *port = &com->ports[i];

		snprintf(text, sizeof(text), "%d (%s) -> n%d [%.03fV]",
		    port->n, port->name, port->node,
		    ES_NodeVoltage(ckt, port->node));
		AG_TlistAddPtr(tl, NULL, text, port);
	}
	pthread_mutex_unlock(&com->lock);
out:
	AG_TlistRestore(tl);
}

static void
PollComponentPairs(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	ES_Component *com;
	int i, j;
	
	AG_TlistClear(tl);
	
	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		if (com->selected)
			break;
	}
	if (com == NULL)
		goto out;

	pthread_mutex_lock(&com->lock);
	for (i = 0; i < com->npairs; i++) {
		char text[AG_TLIST_LABEL_MAX];
		ES_Pair *pair = &com->pairs[i];
		AG_TlistItem *it;

		snprintf(text, sizeof(text), "%s:(%s,%s)",
		    AGOBJECT(com)->name, pair->p1->name, pair->p2->name);
		it = AG_TlistAddPtr(tl, NULL, text, pair);
		it->depth = 0;

		for (j = 0; j < pair->nloops; j++) {
			ES_Loop *dloop = pair->loops[j];
			int dpol = pair->lpols[j];

			snprintf(text, sizeof(text), "%s:L%u (%s)",
			    AGOBJECT(dloop->origin)->name,
			    dloop->name,
			    dpol == 1 ? "+" : "-");
			it = AG_TlistAddPtr(tl, NULL, text, &pair->loops[j]);
			it->depth = 1;
		}
	}
	pthread_mutex_unlock(&com->lock);
out:
	AG_TlistRestore(tl);
}

static void
NewScope(AG_Event *event)
{
	char name[AG_OBJECT_NAME_MAX];
	ES_Circuit *ckt = AG_PTR(1);
	ES_Scope *scope;
	Uint nscope = 0;

tryname:
	snprintf(name, sizeof(name), _("Scope #%u"), nscope++);
	if (AG_ObjectFindChild(ckt, name) != NULL) {
		goto tryname;
	}
	scope = ES_ScopeNew(ckt, name);
	DEV_BrowserOpenData(scope);
}

void
ES_CircuitDrawPort(ES_Circuit *ckt, ES_Port *port, float x, float y)
{
	VG *vg = ckt->vg;
	VG_Element *vge;

	if (ckt->flags & CIRCUIT_SHOW_NODES) {
		vge = VG_Begin(vg, VG_CIRCLE);
		vge->flags |= VG_ELEMENT_SELECTED;
		VG_Vertex2(vg, x, y);
		if (port->selected) {
			VG_ColorRGB(vg, 255, 255, 165);
			VG_CircleRadius(vg, 0.1600);
		} else {
			VG_ColorRGB(vg, 0, 150, 150);
			VG_CircleRadius(vg, 0.0625);
		}
		VG_End(vg);
	}
	if (port->node >= 0) {
		if (ckt->flags &
		    (CIRCUIT_SHOW_NODENAMES|CIRCUIT_SHOW_NODESYMS)) {
			vge = VG_Begin(vg, VG_TEXT);
			vge->flags |= VG_ELEMENT_SELECTED;
			VG_Vertex2(vg, x, y);
			VG_SetStyle(vg, "node-name");
			VG_ColorRGB(vg, 0, 200, 100);
			VG_TextAlignment(vg, VG_ALIGN_BR);

			if ((ckt->flags & CIRCUIT_SHOW_NODESYMS) == 0 &&
			    (ckt->flags & CIRCUIT_SHOW_NODENAMES)) {
				VG_Printf(vg, "n%d", port->node);
			} else {
				char sym[CIRCUIT_SYM_MAX];

				ES_CircuitNodeSymbol(ckt, port->node, sym,
				    sizeof(sym));
				VG_Printf(vg, "%s", sym);
			}
			VG_End(vg);
		}
	}
}

static void
CircuitDrawGeneric(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	ES_Wire *wire;
	int i;

	ES_LockCircuit(ckt);
	VG_SelectBlock(ckt->vg, ckt->vgblk);
	VG_ClearBlock(ckt->vg, ckt->vgblk);
	TAILQ_FOREACH(wire, &ckt->wires, wires) {
		VG_Begin(ckt->vg, VG_LINES);
		VG_Vertex2(ckt->vg, wire->ports[0].x, wire->ports[0].y);
		VG_Vertex2(ckt->vg, wire->ports[1].x, wire->ports[1].y);
		VG_End(ckt->vg);
		for (i = 0; i < 2; i++) {
			ES_Port *port = &wire->ports[i];

			ES_CircuitDrawPort(ckt, &wire->ports[i],
			    port->x, port->y);
		}
	}
	ES_UnlockCircuit(ckt);
}

static void *
Edit(void *p)
{
	ES_Circuit *ckt = p;
	AG_Window *win;
	AG_Toolbar *tbSide;
	AG_Menu *menu;
	AG_MenuItem *mi, *miSub;
	AG_Pane *pane;
	VG_View *vv;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, "%s", AGOBJECT(ckt)->name);

	tbSide = AG_ToolbarNew(NULL, AG_TOOLBAR_VERT, 1, 0);
	vv = VG_ViewNew(NULL, ckt->vg, VG_VIEW_EXPAND);
	VG_ViewDrawFn(vv, CircuitDrawGeneric, "%p", ckt);
	VG_ViewSetSnapMode(vv, VG_GRID);
	VG_ViewSetScale(vv, 30.0f);
	VG_ViewSetGridInterval(vv, 0.5f);
	
	menu = AG_MenuNew(win, AG_MENU_HFILL);
	mi = AG_MenuAddItem(menu, _("File"));
	{
		AG_MenuActionKb(mi, _("Save"), agIconSave.s,
		    SDLK_s, KMOD_CTRL, SaveCircuit, "%p", ckt);
		miSub = AG_MenuNode(mi, _("Export"), agIconSave.s);
		{
			AG_MenuAction(miSub, _("Export to SPICE..."),
			    esIconExportToSpice.s,
			    ExportToSPICE, "%p", ckt);
		}
		AG_MenuSeparator(mi);
		AG_MenuAction(mi, _("Layout settings..."), agIconGear.s,
		    ShowLayoutSettings, "%p,%p,%p", win, ckt, vv);
		AG_MenuSeparator(mi);
		AG_MenuActionKb(mi, _("Close document"), agIconClose.s,
		    SDLK_w, KMOD_CTRL, AGWINCLOSE(win));
	}
	mi = AG_MenuAddItem(menu, _("Edit"));
	{
		/* TODO */
		AG_MenuAction(mi, _("Undo"), NULL, NULL, NULL);
		AG_MenuAction(mi, _("Redo"), NULL, NULL, NULL);
		AG_MenuSeparator(mi);
		miSub = AG_MenuNode(mi, _("Snapping mode"), NULL);
		VG_SnapMenu(menu, miSub, vv);
	}

	mi = AG_MenuAddItem(menu, _("View"));
	{
		AG_MenuAction(mi, _("Create view..."), esIconCircuit.s,
		    CircuitCreateView, "%p,%p", win, ckt);
		AG_MenuSeparator(mi);
		AG_MenuUintFlags(mi, _("Circuit nodes"), esIconNode.s,
		    &ckt->flags, CIRCUIT_SHOW_NODES, 0);
		AG_MenuUintFlags(mi, _("Node names"), esIconNode.s,
		    &ckt->flags, CIRCUIT_SHOW_NODENAMES, 0);
		AG_MenuUintFlags(mi, _("Node symbols"), esIconNode.s,
		    &ckt->flags, CIRCUIT_SHOW_NODESYMS, 0);
		AG_MenuSeparator(mi);
		AG_MenuUintFlags(mi, _("Show grid"), vgIconSnapGrid.s,
		    &ckt->vg->flags, VG_VISGRID, 0);
#ifdef DEBUG
		AG_MenuUintFlags(mi, _("Show extents"), vgIconBlock.s,
		    &ckt->vg->flags, VG_VISBBOXES, 0);
#endif
		AG_MenuSeparator(mi);
		AG_MenuAction(mi, _("Circuit topology..."), esIconMesh.s,
		    ShowTopology, "%p,%p", win, ckt);
		AG_MenuTool(mi, tbSide, _("Log console..."), esIconConsole.s,
		    0, 0,
		    ShowConsole, "%p,%p", win, ckt);
	}
		
	mi = AG_MenuAddItem(menu, _("Simulation"));
	{
		extern const ES_SimOps *esSimOps[];
		const ES_SimOps **pOps, *ops;

		for (pOps = &esSimOps[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			AG_MenuTool(mi, tbSide, _(ops->name),
			    ops->icon ? ops->icon->s : NULL, 0, 0,
			    CircuitSelectSim, "%p,%p,%p", ckt, ops, win);
		}
	}
	
	pane = AG_PaneNewHoriz(win, AG_PANE_EXPAND);
	{
		AG_Notebook *nb;
		AG_NotebookTab *ntab;
		AG_Tlist *tl;
		AG_Box *box;

		nb = AG_NotebookNew(pane->div[0], AG_NOTEBOOK_EXPAND);
		ntab = AG_NotebookAddTab(nb, _("Models"), AG_BOX_VERT);
		{
			char tname[AG_OBJECT_TYPE_MAX];
			extern void *edaModels[];
			void **model;
			int i;

			tl = AG_TlistNew(ntab, AG_TLIST_EXPAND);
			AG_TlistSizeHint(tl, _("Independent voltage source"),
			    20);
			AGWIDGET(tl)->flags &= ~(AG_WIDGET_FOCUSABLE);
			AG_SetEvent(tl, "tlist-dblclick", ES_ComponentInsert,
			    "%p,%p,%p", vv, tl, ckt);
#if 0
			AG_ButtonAct(ntab, AG_BUTTON_HFILL,
			    _("Insert component"),
			    ES_ComponentInsert, "%p,%p,%p", vv, tl, ckt);
#endif
			for (model = &edaModels[0]; *model != NULL; model++)
				AG_TlistAddPtr(tl, NULL,
				    ((ES_ComponentClass *)*model)->name,
				    (void *)*model);
		}

		ntab = AG_NotebookAddTab(nb, _("Objects"), AG_BOX_VERT);
		{
			tl = AG_TlistNew(ntab, AG_TLIST_POLL|AG_TLIST_TREE|
			                       AG_TLIST_EXPAND);
			AG_SetEvent(tl, "tlist-poll", PollCircuitObjs,
			    "%p", ckt);
			AGWIDGET(tl)->flags &= ~(AG_WIDGET_FOCUSABLE);
		}

		ntab = AG_NotebookAddTab(nb, _("Component"), AG_BOX_VERT);
		{
#if 0
			AG_FSpinbutton *fsb;

			label_static(ntab, _("Temperature:"));

			fsb = AG_FSpinbuttonNew(win, 0, "degC",
			    _("Reference: "));
			AG_WidgetBindFloat(fsb, "value", &com->Tnom);
			AG_FSpinbuttonSetMin(fsb, 0.0);
	
			fsb = AG_FSpinbuttonNew(win, 0, "degC",
			    _("Instance: "));
			AG_WidgetBindFloat(fsb, "value", &com->Tspec);
			AG_FSpinbuttonSetMin(fsb, 0.0);

			AG_SeparatorNew(ntab, SEPARATOR_HORIZ);
#endif
			AG_LabelNewStatic(ntab, 0, _("Pinout:"));
			{
				tl = AG_TlistNew(ntab, AG_TLIST_POLL|
						       AG_TLIST_EXPAND);
				AG_SetEvent(tl, "tlist-poll",
				    PollComponentPorts, "%p", ckt);
			}
			AG_LabelNewStatic(ntab, 0, _("Port pairs:"));
			{
				tl = AG_TlistNew(ntab, AG_TLIST_POLL|
				                       AG_TLIST_HFILL);
				AG_SetEvent(tl, "tlist-poll",
				    PollComponentPairs, "%p", ckt);
			}
		}

		box = AG_BoxNew(pane->div[1], AG_BOX_HORIZ, AG_BOX_EXPAND);
		{
			AG_ObjectAttach(box, vv);
			AG_WidgetFocus(vv);
			AG_ObjectAttach(box, tbSide);
		}
	}
	
	mi = AG_MenuAddItem(menu, _("Components"));
	{
		extern VG_ToolOps esInscomOps;
		extern VG_ToolOps esSelcomOps;
		extern VG_ToolOps esWireTool;
		VG_Tool *tool;
			
		VG_ViewButtondownFn(vv, CircuitViewButtondown, "%p", ckt);
		VG_ViewKeydownFn(vv, CircuitViewKeydown, "%p", ckt);

		VG_ViewRegTool(vv, &esInscomOps, ckt);

		tool = VG_ViewRegTool(vv, &esSelcomOps, ckt);
		AG_MenuTool(mi, tbSide, _("Select components"),
		    tool->ops->icon ? tool->ops->icon->s : NULL, 0, 0,
		    CircuitSelectTool, "%p,%p,%p", vv, tool, ckt);
		VG_ViewSetDefaultTool(vv, tool);
		
		tool = VG_ViewRegTool(vv, &esWireTool, ckt);
		AG_MenuTool(mi, tbSide, _("Insert wire"),
		    tool->ops->icon ? tool->ops->icon->s : NULL, 0, 0,
		    CircuitSelectTool, "%p,%p,%p", vv, tool, ckt);
	}
	
	mi = AG_MenuAddItem(menu, _("Visualization"));
	{
		AG_MenuTool(mi, tbSide, _("New scope..."), esIconScope.s,
		    0, 0,
		    NewScope, "%p", ckt);
	}

	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 85, 85);
	return (win);
}

AG_ObjectClass esCircuitClass = {
	"ES_Circuit",
	sizeof(ES_Circuit),
	{ 0,0 },
	Init,
	FreeDataset,
	Destroy,
	Load,
	Save,
	Edit
};
