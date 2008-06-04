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

#include <eda.h>
#include "spice.h"
#include "scope.h"
#include "tools.h"

#include <sources/vsource.h>

extern ES_Component *esFloatingCom;		/* insert_tool.c */
static void InitNode(ES_Node *);
static void InitGround(ES_Circuit *);

/* Resume suspended real-time simulation. */
void
ES_ResumeSimulation(ES_Circuit *ckt)
{
	if (ckt->sim != NULL && ckt->sim->ops->start != NULL)
		ckt->sim->ops->start(ckt->sim);
}

/* Suspend real-time simulation. */
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

	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
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

	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
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
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		for (i = 0; i < com->npairs; i++)
			com->pairs[i].nloops = 0;
	}
	CIRCUIT_FOREACH_VSOURCE(vs, ckt) {
		if (PORT(vs,1)->node == -1 ||		/* XXX why? */
		    PORT(vs,2)->node == -1) {
			continue;
		}
		ES_VsourceFreeLoops(vs);
		ES_VsourceFindLoops(vs);
	}
	if (ckt->loops == NULL) {
		ckt->loops = Malloc(sizeof(ES_Loop *));
	}
	ckt->l = 0;
	CIRCUIT_FOREACH_VSOURCE(vs, ckt) {
		TAILQ_FOREACH(loop, &vs->loops, loops) {
			ckt->loops = Realloc(ckt->loops,
			    (ckt->l+1)*sizeof(ES_Loop *));
			ckt->loops[ckt->l++] = loop;
			loop->name = ckt->l;
		}
	}

	/* Notify the simulation object of the change. */
	if (ckt->sim != NULL &&
	    ckt->sim->ops->cktmod != NULL)
		ckt->sim->ops->cktmod(ckt->sim, ckt);

	/* Update the voltage/current entries in the property table. */
	AG_ObjectFreeProps(OBJECT(ckt));
	for (i = 1; i <= ckt->n; i++) {
		Snprintf(key, sizeof(key), "v%u", i);
		pr = SC_SetReal(ckt, key, 0.0);
		SC_SetRealRdFn(pr, ReadNodeVoltage);
	}
	for (i = 1; i <= ckt->m; i++) {
		Snprintf(key, sizeof(key), "I%u", i);
		pr = SC_SetReal(ckt, key, 0.0);
		SC_SetRealRdFn(pr, ReadBranchCurrent);
	}
	
	/* Notify the component models of the change. */
	CIRCUIT_FOREACH_COMPONENT(com, ckt)
		AG_PostEvent(ckt, com, "circuit-modified", NULL);
}

static void
PreSimulationStep(AG_Event *event)
{
	ES_Circuit *ckt = AG_SELF();
	AG_Object *obj;

	OBJECT_FOREACH_CHILD(obj, ckt, ag_object)
		AG_PostEvent(ckt, obj, "circuit-step-begin", NULL);
}

static void
PostSimulationStep(AG_Event *event)
{
	ES_Circuit *ckt = AG_SELF();
	AG_Object *obj;

	OBJECT_FOREACH_CHILD(obj, ckt, ag_object)
		AG_PostEvent(ckt, obj, "circuit-step-end", NULL);
}

ES_Circuit *
ES_CircuitNew(void *parent, const char *name)
{
	ES_Circuit *ckt;

	ckt = Malloc(sizeof(ES_Circuit));
	AG_ObjectInit(ckt, &esCircuitClass);
	AG_ObjectSetName(ckt, "%s", name);
	AG_ObjectAttach(parent, ckt);
	return (ckt);
}

static void
Init(void *p)
{
	ES_Circuit *ckt = p;

	ckt->flags = CIRCUIT_SHOW_NODES|CIRCUIT_SHOW_NODESYMS;
	ckt->simlock = 0;
	ckt->descr[0] = '\0';
	ckt->authors[0] = '\0';
	ckt->keywords[0] = '\0';
	ckt->sim = NULL;
	ckt->loops = NULL;
	ckt->vsrcs = NULL;
	ckt->m = 0;
	ckt->nodes = Malloc(sizeof(ES_Node *));
	ckt->n = 0;
	ckt->console = NULL;
	TAILQ_INIT(&ckt->syms);
	InitGround(ckt);

	ckt->vg = VG_New(0);
	Strlcpy(ckt->vg->layers[0].name, _("Schematic"),
	    sizeof(ckt->vg->layers[0].name));
	
	AG_SetEvent(ckt, "edit-open", EditOpen, NULL);
	AG_SetEvent(ckt, "edit-close", EditClose, NULL);

	/* Notify visualization objects of simulation progress. */
	AG_SetEvent(ckt, "circuit-step-begin", PreSimulationStep, NULL);
	AG_SetEvent(ckt, "circuit-step-end", PostSimulationStep, NULL);
}

static void
FreeNode(ES_Node *node)
{
	ES_Branch *br, *nbr;

	for (br = TAILQ_FIRST(&node->branches);
	     br != TAILQ_END(&node->branches);
	     br = nbr) {
		nbr = TAILQ_NEXT(br, branches);
		Free(br);
	}
	TAILQ_INIT(&node->branches);
	Free(node);
}

static void
FreeDataset(void *p)
{
	ES_Circuit *ckt = p;
	ES_Component *com;
	ES_Port *port;
	ES_Sym *sym, *nsym;
	Uint i;

	ES_DestroySimulation(ckt);

	Free(ckt->loops);
	ckt->loops = NULL;
	ckt->l = 0;

	Free(ckt->vsrcs);
	ckt->vsrcs = NULL;
	ckt->m = 0;

	for (sym = TAILQ_FIRST(&ckt->syms);
	     sym != TAILQ_END(&ckt->syms);
	     sym = nsym) {
		nsym = TAILQ_NEXT(sym, syms);
		Free(sym);
	}
	TAILQ_INIT(&ckt->syms);

	for (i = 0; i <= ckt->n; i++) {
		FreeNode(ckt->nodes[i]);
	}
	ckt->nodes = Realloc(ckt->nodes, sizeof(ES_Node *));
	ckt->n = 0;
	InitGround(ckt);

	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		COMPONENT_FOREACH_PORT(port, i, com) {
			port->node = -1;
			port->branch = NULL;
			port->flags = 0;
		}
	}
	VG_Reinit(ckt->vg);
}

static int
Load(void *p, AG_DataSource *ds, const AG_Version *ver)
{
	ES_Circuit *ckt = p;
	ES_Component *com;
	Uint32 nitems;
	Uint i, j, nnodes;

	AG_CopyString(ckt->descr, ds, sizeof(ckt->descr));
	AG_CopyString(ckt->authors, ds, sizeof(ckt->authors));
	AG_CopyString(ckt->keywords, ds, sizeof(ckt->keywords));
	ckt->flags = AG_ReadUint32(ds);

	/* Load the circuit nodes. */
	nnodes = (Uint)AG_ReadUint32(ds);
	for (i = 0; i <= nnodes; i++) {
		int nBranches, flags;
		int name;

		flags = (int)AG_ReadUint32(ds);
		nBranches = (int)AG_ReadUint32(ds);
		if (i == 0) {
			name = 0;
			FreeNode(ckt->nodes[0]);
			InitGround(ckt);
		} else {
			name = ES_AddNode(ckt);
			ckt->nodes[name]->flags = flags & ~(CKTNODE_EXAM);
		}

		for (j = 0; j < nBranches; j++) {
			char ppath[AG_OBJECT_PATH_MAX];
			ES_Component *pcom;
			ES_Branch *br;
			int pport;

			AG_CopyString(ppath, ds, sizeof(ppath));
			AG_ReadUint32(ds);			/* Pad */
			pport = (int)AG_ReadUint32(ds);
			if ((pcom = AG_ObjectFind(ckt, ppath)) == NULL) {
				AG_SetError("%s", AG_GetError());
				return (-1);
			}
			if (pport > pcom->nports) {
				AG_SetError("%s: port #%d > %d", ppath, pport,
				    pcom->nports);
				return (-1);
			}
			br = ES_AddBranch(ckt, name, &pcom->ports[pport]);
			pcom->ports[pport].node = name;
			pcom->ports[pport].branch = br;
		}
	}

	/* Load the schematics. */
	if (VG_Load(ckt->vg, ds) == -1)
		return (-1);

	/* Load the symbol table. */
	nitems = AG_ReadUint32(ds);
	for (i = 0; i < nitems; i++) {
		char name[CIRCUIT_SYM_MAX];
		ES_Sym *sym;
		
		sym = ES_AddSymbol(ckt, NULL);
		AG_CopyString(sym->name, ds, sizeof(sym->name));
		AG_CopyString(sym->descr, ds, sizeof(sym->descr));
		sym->type = (enum es_sym_type)AG_ReadUint32(ds);
		AG_ReadUint32(ds);				/* Pad */
		switch (sym->type) {
		case ES_SYM_NODE:
			sym->p.node = (int)AG_ReadSint32(ds);
			break;
		case ES_SYM_VSOURCE:
			sym->p.vsource = (int)AG_ReadSint32(ds);
			break;
		case ES_SYM_ISOURCE:
			sym->p.isource = (int)AG_ReadSint32(ds);
			break;
		}
	}

	/* Load the component port assignments. */
	nitems = AG_ReadUint32(ds);
	for (i = 0; i < nitems; i++) {
		char name[AG_OBJECT_NAME_MAX];
		int j;
		ES_Component *com;
		ES_Port *port;

		/* Lookup the component. */
		AG_CopyString(name, ds, sizeof(name));
		CIRCUIT_FOREACH_COMPONENT(com, ckt) {
			if (strcmp(OBJECT(com)->name, name) == 0)
				break;
		}
		if (com == NULL) { Fatal("unexisting component"); }
		
		/* Load the port information. */
		com->nports = (int)AG_ReadUint32(ds);
		COMPONENT_FOREACH_PORT(port, j, com) {
			ES_Branch *br;
			ES_Node *node;

			port->n = (int)AG_ReadUint32(ds);
			AG_CopyString(port->name, ds, sizeof(port->name));
			port->node = (int)AG_ReadUint32(ds);
			node = ckt->nodes[port->node];

			NODE_FOREACH_BRANCH(br, node) {
				if (br->port->com == com &&
				    br->port->n == port->n)
					break;
			}
			if (br == NULL) { Fatal("Illegal branch"); }
			port->branch = br;
			port->flags = 0;
		}
	}

	/* Send circuit-connected event to components. */
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		AG_PostEvent(ckt, com, "circuit-connected", NULL);
	}
	ES_CircuitModified(ckt);
	return (0);
}

static int
Save(void *p, AG_DataSource *ds)
{
	char path[AG_OBJECT_PATH_MAX];
	ES_Circuit *ckt = p;
	ES_Node *node;
	ES_Branch *br;
	ES_Component *com;
	ES_Sym *sym;
	off_t count_offs;
	Uint32 count;
	Uint i;

	AG_WriteString(ds, ckt->descr);
	AG_WriteString(ds, ckt->authors);
	AG_WriteString(ds, ckt->keywords);
	AG_WriteUint32(ds, ckt->flags);
	AG_WriteUint32(ds, ckt->n);
	for (i = 0; i <= ckt->n; i++) {
		ES_Node *node = ckt->nodes[i];

		AG_WriteUint32(ds, (Uint32)node->flags);
		count_offs = AG_Tell(ds);
		count = 0;
		AG_WriteUint32(ds, 0);
		NODE_FOREACH_BRANCH(br, node) {
			if (br->port == NULL || br->port->com == NULL ||
			    COMPONENT_IS_FLOATING(br->port->com)) {
				continue;
			}
			AG_ObjectCopyName(br->port->com, path, sizeof(path));
			AG_WriteString(ds, path);
			AG_WriteUint32(ds, 0);			/* Pad */
			AG_WriteUint32(ds, (Uint32)br->port->n);
			count++;
		}
		AG_WriteUint32At(ds, count, count_offs);
	}
	
	/* Save the schematics. */
	VG_Save(ckt->vg, ds);

	/* Save the symbol table. */
	count_offs = AG_Tell(ds);
	count = 0;
	AG_WriteUint32(ds, 0);
	TAILQ_FOREACH(sym, &ckt->syms, syms) {
		AG_WriteString(ds, sym->name);
		AG_WriteString(ds, sym->descr);
		AG_WriteUint32(ds, (Uint32)sym->type);
		AG_WriteUint32(ds, 0);				/* Pad */
		switch (sym->type) {
		case ES_SYM_NODE:
			AG_WriteSint32(ds, (Sint32)sym->p.node);
			break;
		case ES_SYM_VSOURCE:
			AG_WriteSint32(ds, (Sint32)sym->p.vsource);
			break;
		case ES_SYM_ISOURCE:
			AG_WriteSint32(ds, (Sint32)sym->p.isource);
			break;
		}
		count++;
	}
	AG_WriteUint32At(ds, count, count_offs);

	/* Save the component information. */
	count_offs = AG_Tell(ds);
	count = 0;
	AG_WriteUint32(ds, 0);
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		ES_Port *port;

		AG_WriteString(ds, OBJECT(com)->name);
		AG_WriteUint32(ds, (Uint32)com->nports);
		COMPONENT_FOREACH_PORT(port, i, com) {
			AG_WriteUint32(ds, (Uint32)port->n);
			AG_WriteString(ds, port->name);
			AG_WriteUint32(ds, (Uint32)port->node);
		}
		count++;
	}
	AG_WriteUint32At(ds, count, count_offs);
	return (0);
}

static void
InitNode(ES_Node *node)
{
	node->flags = 0;
	node->nBranches = 0;
	TAILQ_INIT(&node->branches);
}

static void
InitGround(ES_Circuit *ckt)
{
	ckt->nodes[0] = Malloc(sizeof(ES_Node));
	InitNode(ckt->nodes[0]);
	ckt->nodes[0]->flags |= CKTNODE_REFERENCE;
	ES_AddNodeSymbol(ckt, "Gnd", 0);
}

/* Create a new node with no branches. */
int
ES_AddNode(ES_Circuit *ckt)
{
	ES_Node *node;
	int n = ++ckt->n;

	ckt->nodes = Realloc(ckt->nodes, (n+1)*sizeof(ES_Node *));
	ckt->nodes[n] = Malloc(sizeof(ES_Node));
	InitNode(ckt->nodes[n]);
	return (n);
}

/* Create a new symbol referencing a node. */
ES_Sym *
ES_AddNodeSymbol(ES_Circuit *ckt, const char *name, int node)
{
	ES_Sym *sym;

	sym = ES_AddSymbol(ckt, name);
	sym->type = ES_SYM_NODE;
	sym->p.node = node;
	return (sym);
}

/* Create a new symbol referencing a voltage source. */
ES_Sym *
ES_AddVsourceSymbol(ES_Circuit *ckt, const char *name, int vsource)
{
	ES_Sym *sym;

	sym = ES_AddSymbol(ckt, name);
	sym->type = ES_SYM_VSOURCE;
	sym->p.vsource = vsource;
	return (sym);
}

/* Create a new symbol referencing a current source. */
ES_Sym *
ES_AddIsourceSymbol(ES_Circuit *ckt, const char *name, int isource)
{
	ES_Sym *sym;

	sym = ES_AddSymbol(ckt, name);
	sym->type = ES_SYM_ISOURCE;
	sym->p.vsource = isource;
	return (sym);
}

/* Create a new symbol. */
ES_Sym *
ES_AddSymbol(ES_Circuit *ckt, const char *name)
{
	ES_Sym *sym;

	sym = Malloc(sizeof(ES_Sym));
	if (name != NULL) {
		Strlcpy(sym->name, name, sizeof(sym->name));
	} else {
		sym->name[0] = '\0';
	}
	sym->descr[0] = '\0';
	sym->type = 0;
	sym->p.node = 0;
	TAILQ_INSERT_TAIL(&ckt->syms, sym, syms);
	return (sym);
}

/* Delete a symbol. */
void
ES_DelSymbol(ES_Circuit *ckt, ES_Sym *sym)
{
	TAILQ_REMOVE(&ckt->syms, sym, syms);
	Free(sym);
}

/* Lookup a symbol by name. */
ES_Sym *
ES_LookupSymbol(ES_Circuit *ckt, const char *name)
{
	ES_Sym *sym;

	TAILQ_FOREACH(sym, &ckt->syms, syms) {
		if (strcmp(sym->name, name) == 0)
			break;
	}
	if (sym == NULL) {
		AG_SetError(_("%s: no such symbol: `%s'"),
		    OBJECT(ckt)->name, name);
	}
	return (sym);
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

	NODE_FOREACH_BRANCH(br, node) {
		if (br->port == NULL ||
		   (vs = VSOURCE(br->port->com)) == NULL) {
			continue;
		}
		if (COMPONENT_IS_FLOATING(vs) ||
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

/* Lookup an existing node by number or die. */
ES_Node *
ES_GetNode(ES_Circuit *ckt, int n)
{
	if (n < 0 || n > ckt->n) {
		Fatal("%s:%d: Bad node (Circuit=%d)", OBJECT(ckt)->name, n,
		    ckt->n);
	}
	return (ckt->nodes[n]);
}

/* Return the matching symbol (or "nX") for a given node. */
/* XXX multiple symbols are possible */
void
ES_CopyNodeSymbol(ES_Circuit *ckt, int n, char *dst, size_t dst_len)
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
	Snprintf(dst, dst_len, "n%d", n);
	return;
}

/* Search for a node matching the given name (symbol or "nX") */
/* XXX multiple symbols are possible */
ES_Node *
ES_GetNodeBySymbol(ES_Circuit *ckt, const char *name)
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

/*
 * Remove a node and all references to it. If necessary, shift the entire
 * node array and update all port references accordingly.
 */
void
ES_DelNode(ES_Circuit *ckt, int n)
{
	ES_Branch *br;
	int i;

	ES_CircuitLog(ckt, _("Deleting n%d"), n);
#ifdef DEBUG
	if (n == 0 || n > ckt->n)
		Fatal("Cannot delete n%d (max %d)", n, ckt->n);
#endif
	if (n != ckt->n) {
		/* Update the Branch port pointers. */
		for (i = n; i <= ckt->n; i++) {
			NODE_FOREACH_BRANCH(br, ckt->nodes[i]) {
				if (br->port != NULL && br->port->com != NULL) {
					ES_ComponentLog(br->port->com,
					    _("Updating branch: %s: "
					      "n%d -> n%d"),
					    br->port->name,
					    br->port->node, i-1);
					br->port->node = i-1;
				}
			}
		}
		FreeNode(ckt->nodes[n]);
		memmove(&ckt->nodes[n], &ckt->nodes[n+1],
		    (ckt->n - n) * sizeof(ES_Node *));
	} else {
		FreeNode(ckt->nodes[n]);
	}
	ckt->n--;
}

/*
 * Merge the branches of two nodes, creating a new node from them. The
 * new node will always use whichever of the two node names is the lowest
 * numbered. The reference node (0) is never deleted.
 */
int
ES_MergeNodes(ES_Circuit *ckt, int N1, int N2)
{
	ES_Branch *br;
	int Nnew, Nold;

	if (N1 < N2) {
		Nnew = N1;
		Nold = N2;
	} else {
		Nnew = N2;
		Nold = N1;
	}
	ES_CircuitLog(ckt, _("Merging: n%d,n%d -> n%d"), N1, N2, Nnew);
rescan:
	NODE_FOREACH_BRANCH(br, ckt->nodes[Nold]) {
		ES_Port *brPort = br->port;
		brPort->node = Nnew;
		ES_AddBranch(ckt, Nnew, brPort);
		ES_DelBranch(ckt, Nold, br);
		goto rescan;
	}
	ES_DelNode(ckt, Nold);
	return (Nnew);
}

/* Create a new branch, connecting a Circuit Node to a Component Port. */
ES_Branch *
ES_AddBranch(ES_Circuit *ckt, int n, ES_Port *port)
{
	ES_Node *node = ckt->nodes[n];
	ES_Branch *br;

	br = Malloc(sizeof(ES_Branch));
	br->port = port;
	TAILQ_INSERT_TAIL(&node->branches, br, branches);
	node->nBranches++;
	return (br);
}

/* Lookup the branch connecting the given Node to the given Port. */
ES_Branch *
ES_LookupBranch(ES_Circuit *ckt, int n, ES_Port *port)
{
	ES_Node *node = ckt->nodes[n];
	ES_Branch *br;

	NODE_FOREACH_BRANCH(br, node) {
		if (br->port == port)
			break;
	}
	return (br);
}

/* Remove the specified branch. */
void
ES_DelBranch(ES_Circuit *ckt, int n, ES_Branch *br)
{
	ES_Node *node = ckt->nodes[n];

	TAILQ_REMOVE(&node->branches, br, branches);
	Free(br);
#ifdef DEBUG
	if ((node->nBranches - 1) < 0) { Fatal("--nBranches < 0"); }
#endif
	node->nBranches--;
}

/* Stop any simulation in progress and free the simulation object. */
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
		AG_WindowSetCaption(sim->win, "%s: %s", OBJECT(ckt)->name,
		    _(sim->ops->name));
		AG_WindowSetPosition(sim->win, AG_WINDOW_LOWER_LEFT, 0);
		AG_WindowShow(sim->win);
	}
	return (sim);
}

/* Log a message to the Circuit's log console. */
void
ES_CircuitLog(void *p, const char *fmt, ...)
{
	ES_Circuit *ckt = p;
	AG_ConsoleLine *ln;
	va_list args;

	va_start(args, fmt);
#ifdef DEBUG
	fprintf(stderr, "%s: ", OBJECT(ckt)->name);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
#endif
	if (ckt->console != NULL) {
		ln = AG_ConsoleAppendLine(ckt->console, NULL);
		AG_Vasprintf(&ln->text, fmt, args);
		ln->len = strlen(ln->text);
	}
	va_end(args);
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
		it = AG_TlistAdd(tl, NULL, "%s:L%u (%s)", OBJECT(vs)->name,
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
		    node->flags, node->nBranches);
		it->p1 = node;
		it->depth = 0;
		NODE_FOREACH_BRANCH(br, node) {
			if (br->port == NULL) {
				it = AG_TlistAdd(tl, NULL, "(null port)");
				it->p1 = br;
				it->depth = 1;
			} else {
				it = AG_TlistAdd(tl, NULL, "%s:%s(%d)",
				    OBJECT(br->port->com)->name,
				    br->port->name, br->port->n);
				it->p1 = br;
				it->depth = 1;
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

		it = AG_TlistAdd(tl, NULL, "%s", OBJECT(vs)->name);
		it->p1 = vs;
	}
	AG_TlistRestore(tl);
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
	
	if ((win = AG_WindowNewNamed(0, "%s-topology", OBJECT(ckt)->name))
	    == NULL) {
		return;
	}
	AG_WindowSetCaption(win, _("%s: Circuit topology"), OBJECT(ckt)->name);
	
	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);
	ntab = AG_NotebookAddTab(nb, _("Nodes"), AG_BOX_VERT);
	{
		AG_LabelNewPolledMT(ntab, 0, &OBJECT(ckt)->lock,
		    _("%u+1 nodes, %u vsources, %u loops"),
		    &ckt->n, &ckt->m, &ckt->l);
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
ShowProperties(AG_Event *event)
{
	char path[AG_OBJECT_PATH_MAX];
	AG_Window *pwin = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	VG_View *vv = AG_PTR(3);
	AG_Window *win;
	AG_Textbox *tb;
	
	AG_ObjectCopyName(ckt, path, sizeof(path));
	if ((win = AG_WindowNewNamed(0, "settings-%s", path)) == NULL) {
		return;
	}
	AG_WindowSetCaption(win, _("Circuit properties: %s"),
	    OBJECT(ckt)->name);
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 40, 30);
	
	tb = AG_TextboxNew(win, AG_TEXTBOX_HFILL, _("Author: "));
	AG_TextboxBindUTF8(tb, ckt->authors, sizeof(ckt->authors));

	tb = AG_TextboxNew(win, AG_TEXTBOX_HFILL, _("Keywords: "));
	AG_TextboxBindUTF8(tb, ckt->keywords, sizeof(ckt->keywords));

	AG_LabelNew(win, 0, _("Description: "));
	tb = AG_TextboxNew(win, AG_TEXTBOX_EXPAND|AG_TEXTBOX_MULTILINE|
	                        AG_TEXTBOX_CATCH_TAB, NULL);
	AG_TextboxBindUTF8(tb, ckt->descr, sizeof(ckt->descr));
	AG_WidgetFocus(tb);

	AG_ButtonNewFn(win, AG_BUTTON_HFILL, _("Close"), AGWINCLOSE(win));
	AG_WindowAttach(pwin, win);
	AG_WindowShow(win);
}

static void
ExportToSPICE(AG_Event *event)
{
	char name[FILENAME_MAX];
	ES_Circuit *ckt = AG_PTR(1);

	Strlcpy(name, OBJECT(ckt)->name, sizeof(name));
	Strlcat(name, ".cir", sizeof(name));

	if (ES_CircuitExportSPICE3(ckt, name) == -1)
		AG_TextMsg(AG_MSG_ERROR, "%s: %s", OBJECT(ckt)->name,
		    AG_GetError());
}

static void
ViewButtonDown(AG_Event *event)
{
	VG_View *vv =  AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	int button = AG_INT(2);
	float x = AG_FLOAT(3);
	float y = AG_FLOAT(4);
	ES_Component *com;
//	VG_Block *block;

	if (button != SDL_BUTTON_RIGHT) {
		return;
	}
#if 0
	block = VG_BlockClosest(ckt->vg, x, y);

	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (OBJECT(com)->cls->edit == NULL ||
		    com->block != block) {
			continue;
		}
		ES_ComponentMenu(com, vv);
		break;
	}
#endif
}

static void
ViewKeyDown(AG_Event *event)
{
	VG_View *vv =  AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
}

#if 0
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

	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (OBJECT(com)->cls->edit == NULL ||
		    com->block != block) {
			continue;
		}
		DEV_BrowserOpenData(com);
		break;
	}
}
#endif

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
		    OBJECT(ckt)->name);
		return;
	}

	sim = ES_SetSimulationMode(ckt, sops);
	if (sim->win != NULL)
		AG_WindowAttach(pwin, sim->win);
}

static void
FindCircuitObjs(AG_Tlist *tl, AG_Object *pob, int depth, void *ckt)
{
	AG_Object *cob;
	AG_TlistItem *it;

	it = AG_TlistAdd(tl, NULL, "%s%s", pob->name,
	    (pob->flags & AG_OBJECT_RESIDENT) ? " (resident)" : "");
	it->depth = depth;
	it->cat = "object";
	it->p1 = pob;

	if (!TAILQ_EMPTY(&pob->children)) {
		it->flags |= AG_TLIST_HAS_CHILDREN;
		if (pob == OBJECT(ckt))
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

	if ((win = AG_WindowNewNamed(0, "%s-console", OBJECT(ckt)->name))
	    == NULL) {
		return;
	}
	AG_WindowSetCaption(win, _("Console: %s"), OBJECT(ckt)->name);
	ckt->console = AG_ConsoleNew(win, AG_CONSOLE_EXPAND);
	AG_WidgetFocus(ckt->console);
	AG_WindowAttach(pwin, win);

	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_BL, 50, 30);
	AG_WindowShow(win);
}

static void
CreateView(AG_Event *event)
{
	AG_Window *pwin = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	VG_View *vv;
	AG_Window *win;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("View of %s"), OBJECT(ckt)->name);

	vv = VG_ViewNew(win, ckt->vg, VG_VIEW_EXPAND);
	AG_WidgetFocus(vv);
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_TR, 60, 50);
	AG_WindowAttach(pwin, win);
	AG_WindowShow(win);
}

static void
PollComponentPorts(AG_Event *event)
{
	char text[AG_TLIST_LABEL_MAX];
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	ES_Component *com;
	ES_Port *port;
	int i;
	
	AG_TlistClear(tl);
	
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->selected)
			break;
	}
	if (com == NULL)
		goto out;

	AG_ObjectLock(com);
	COMPONENT_FOREACH_PORT(port, i, com) {
		Snprintf(text, sizeof(text),
		    "%d (%s) -> n%d [%.03fV]",
		    port->n, port->name, port->node,
		    ES_NodeVoltage(ckt, port->node));
		AG_TlistAddPtr(tl, NULL, text, port);
	}
	AG_ObjectUnlock(com);
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
	
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->selected)
			break;
	}
	if (com == NULL)
		goto out;

	AG_ObjectLock(com);
	for (i = 0; i < com->npairs; i++) {
		char text[AG_TLIST_LABEL_MAX];
		ES_Pair *pair = &com->pairs[i];
		AG_TlistItem *it;

		Snprintf(text, sizeof(text), "%s:(%s,%s)",
		    OBJECT(com)->name, pair->p1->name, pair->p2->name);
		it = AG_TlistAddPtr(tl, NULL, text, pair);
		it->depth = 0;

		for (j = 0; j < pair->nloops; j++) {
			ES_Loop *dloop = pair->loops[j];
			int dpol = pair->lpols[j];

			Snprintf(text, sizeof(text), "%s:L%u (%s)",
			    OBJECT(dloop->origin)->name,
			    dloop->name,
			    dpol == 1 ? "+" : "-");
			it = AG_TlistAddPtr(tl, NULL, text, &pair->loops[j]);
			it->depth = 1;
		}
	}
	AG_ObjectUnlock(com);
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
	Snprintf(name, sizeof(name), _("Scope #%u"), nscope++);
	if (AG_ObjectFindChild(ckt, name) != NULL) {
		goto tryname;
	}
	scope = ES_ScopeNew(ckt, name);
	ES_CreateEditionWindow(scope);
}

static void
InsertComponent(AG_Event *event)
{
	char name[AG_OBJECT_NAME_MAX];
	VG_View *vgv = AG_PTR(1);
	AG_Tlist *tl = AG_PTR(2);
	ES_Circuit *ckt = AG_PTR(3);
	AG_TlistItem *it;
	ES_ComponentClass *cls;
	ES_Component *com;
	VG_Tool *t;
	int n = 1;

	if ((it = AG_TlistSelectedItem(tl)) == NULL) {
		AG_TextMsg(AG_MSG_ERROR, _("No component type is selected."));
		return;
	}
	cls = (ES_ComponentClass *)it->p1;
tryname:
	Snprintf(name, sizeof(name), "%s%d", cls->pfx, n++);
	CIRCUIT_FOREACH_COMPONENT_ALL(com, ckt) {
		if (strcmp(OBJECT(com)->name, name) == 0)
			break;
	}
	if (com != NULL)
		goto tryname;

	com = Malloc(cls->obj.size);
	AG_ObjectInit(com, cls);
	AG_ObjectSetName(com, "%s", name);
	com->flags |= COMPONENT_FLOATING;

	ES_LockCircuit(ckt);

	AG_ObjectAttach(ckt, com);
	AG_ObjectUnlinkDatafiles(com);
	AG_ObjectPageIn(com);
	AG_PostEvent(ckt, com, "circuit-shown", NULL);

	if ((t = VG_ViewFindTool(vgv, "Insert component")) != NULL) {
		VG_ViewSelectTool(vgv, t, ckt);
		esFloatingCom = com;
		esFloatingCom->selected = 1;
	}
//	AG_WidgetFocus(vgv);
//	AG_WindowFocus(AG_WidgetParentWindow(vgv));
//	if (AG_ObjectSave(com) == -1)
//		AG_TextMsg(AG_MSG_ERROR, "%s", AG_GetError());

	ES_UnlockCircuit(ckt);
}

static void
FindSchemTree(AG_Tlist *tl, VG_Node *node, int depth)
{
	AG_TlistItem *it;
	VG_Node *cNode;

	it = AG_TlistAdd(tl, NULL, "%s%u", node->ops->name, node->handle);
	it->depth = depth;
	it->p1 = node;
	it->selected = (node->flags & VG_NODE_SELECTED);

	if (!TAILQ_EMPTY(&node->cNodes)) {
		it->flags |= AG_TLIST_HAS_CHILDREN;
	}
	if ((it->flags & AG_TLIST_HAS_CHILDREN) &&
	    AG_TlistVisibleChildren(tl, it)) {
		TAILQ_FOREACH(cNode, &node->cNodes, tree)
			FindSchemTree(tl, cNode, depth+1);
	}
}

static void
PollSchemTree(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);

	AG_TlistBegin(tl);
	FindSchemTree(tl, (VG_Node *)ckt->vg->root, 0);
	AG_TlistEnd(tl);
}

static void
PollSchemList(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	AG_TlistItem *it;
	VG_Node *vn;

	AG_TlistBegin(tl);
	VG_FOREACH_NODE(vn, ckt->vg, vg_node) {
		it = AG_TlistAdd(tl, NULL, "%s%u", vn->ops->name, vn->handle);
		it->selected = (vn->flags & VG_NODE_SELECTED);
		it->p1 = vn;
	}
	AG_TlistEnd(tl);
}

static void *
Edit(void *p)
{
	ES_Circuit *ckt = p;
	AG_Window *win;
	AG_Toolbar *tbTop, *tbRight;
	AG_Menu *menu;
	AG_MenuItem *mi, *miSub;
	AG_Pane *pane;
	VG_View *vv;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, "%s", OBJECT(ckt)->name);

	tbTop = AG_ToolbarNew(NULL, AG_TOOLBAR_HORIZ, 1, 0);
	tbRight = AG_ToolbarNew(NULL, AG_TOOLBAR_VERT, 1, 0);
	vv = VG_ViewNew(NULL, ckt->vg, VG_VIEW_EXPAND|VG_VIEW_GRID);
	VG_ViewSetSnapMode(vv, VG_GRID);
	VG_ViewSetScale(vv, 1);
	
	menu = AG_MenuNew(win, AG_MENU_HFILL);

	mi = AG_MenuAddItem(menu, _("File"));
	{
		AG_MenuTool(mi, tbTop, _("Properties..."), agIconGear.s,
		    0, 0,
		    ShowProperties, "%p,%p,%p", win, ckt, vv);
		AG_MenuActionKb(mi, _("Close document"), agIconClose.s,
		    SDLK_w, KMOD_CTRL,
		    AGWINCLOSE(win));
	}

	mi = AG_MenuAddItem(menu, _("Edit"));
	{
		miSub = AG_MenuNode(mi, _("Snapping mode"), NULL);
		VG_SnapMenu(menu, miSub, vv);
	}
	mi = AG_MenuAddItem(menu, _("View"));
	{
		AG_MenuActionKb(mi, _("New view..."), esIconCircuit.s,
		    SDLK_v, KMOD_CTRL,
		    CreateView, "%p,%p", win, ckt);
		AG_MenuSeparator(mi);
		AG_MenuUintFlags(mi, _("Show circuit nodes"), esIconNode.s,
		    &ckt->flags, CIRCUIT_SHOW_NODES, 0);
		AG_MenuUintFlags(mi, _("Show node names"), esIconNode.s,
		    &ckt->flags, CIRCUIT_SHOW_NODENAMES, 0);
		AG_MenuUintFlags(mi, _("Show node symbols"), esIconNode.s,
		    &ckt->flags, CIRCUIT_SHOW_NODESYMS, 0);
		AG_MenuSeparator(mi);
		AG_MenuUintFlags(mi, _("Show grid"), vgIconSnapGrid.s,
		    &vv->flags, VG_VIEW_GRID, 0);
		AG_MenuUintFlags(mi, _("Show construction geometry"),
		    esIconConstructionGeometry.s,
		    &vv->flags, VG_VIEW_CONSTRUCTION, 0);
#ifdef DEBUG
		AG_MenuUintFlags(mi, _("Show extents"), vgIconBlock.s,
		    &vv->flags, VG_VIEW_EXTENTS, 0);
#endif
		AG_MenuSeparator(mi);
		AG_MenuAction(mi, _("Circuit topology..."), esIconMesh.s,
		    ShowTopology, "%p,%p", win, ckt);
		AG_MenuTool(mi, tbTop, _("Log console..."), esIconConsole.s,
		    0, 0,
		    ShowConsole, "%p,%p", win, ckt);
	}
	
	mi = AG_MenuAddItem(menu, _("Simulation"));
	{
		extern const ES_SimOps *esSimOps[];
		const ES_SimOps **pOps, *ops;

		for (pOps = &esSimOps[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			AG_MenuTool(mi, tbTop, _(ops->name),
			    ops->icon ? ops->icon->s : NULL, 0,0,
			    CircuitSelectSim, "%p,%p,%p", ckt, ops, win);
		}
	}
	
	pane = AG_PaneNewHoriz(win, AG_PANE_EXPAND);
	{
		AG_Notebook *nb;
		AG_NotebookTab *ntab;
		AG_Tlist *tl;
		AG_Box *box, *box2;

		nb = AG_NotebookNew(pane->div[0], AG_NOTEBOOK_EXPAND);
		ntab = AG_NotebookAddTab(nb, _("Models"), AG_BOX_VERT);
		{
			char tname[AG_OBJECT_TYPE_MAX];
			void **cc;
			int i;

			tl = AG_TlistNew(ntab, AG_TLIST_EXPAND);
			AG_TlistSizeHint(tl, _("Independent voltage source"),
			    20);
			AGWIDGET(tl)->flags &= ~(AG_WIDGET_FOCUSABLE);
			AG_SetEvent(tl, "tlist-dblclick", InsertComponent,
			    "%p,%p,%p", vv, tl, ckt);
			for (cc = &esComponentClasses[0]; *cc != NULL; cc++)
				AG_TlistAddPtr(tl, NULL,
				    ((ES_ComponentClass *)*cc)->name,
				    (void *)*cc);
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
		
		ntab = AG_NotebookAddTab(nb, _("Schematics"), AG_BOX_VERT);
		AG_BoxSetHomogenous(AGBOX(ntab), 1);
		{
			tl = AG_TlistNewPolled(ntab,
			    AG_TLIST_TREE|AG_TLIST_HFILL|AG_TLIST_NOSELSTATE,
			    PollSchemTree, "%p", ckt);
			AG_TlistSizeHint(tl, "<Point1234>", 4);
			
			tl = AG_TlistNewPolled(ntab,
			    AG_TLIST_HFILL|AG_TLIST_NOSELSTATE,
			    PollSchemList, "%p", ckt);
			AG_TlistSizeHint(tl, "<Point1234>", 4);
		}

		box = AG_BoxNewVert(pane->div[1], AG_BOX_EXPAND);
		AG_BoxSetSpacing(box, 0);
		AG_BoxSetPadding(box, 0);
		{
			AG_ObjectAttach(box, tbTop);
			box2 = AG_BoxNewHoriz(box, AG_BOX_EXPAND);
			AG_BoxSetSpacing(box2, 0);
			AG_BoxSetPadding(box2, 0);
			{
				AG_ObjectAttach(box2, vv);
				AG_ObjectAttach(box2, tbRight);
				AG_WidgetFocus(vv);
			}
		}
	}
	
	mi = AG_MenuAddItem(menu, _("Components"));
	{
		VG_ToolOps **pOps, *ops;
		VG_Tool *tool;

		for (pOps = &esCircuitTools[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			tool = VG_ViewRegTool(vv, ops, ckt);
			AG_MenuTool(mi, tbRight, ops->name,
			    ops->icon ? ops->icon->s : NULL, 0,0,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, ckt);
		}
		VG_ViewRegTool(vv, &esInsertTool, ckt);
		VG_ViewButtondownFn(vv, ViewButtonDown, "%p", ckt);
		VG_ViewKeydownFn(vv, ViewKeyDown, "%p", ckt);
	}
	
	mi = AG_MenuAddItem(menu, _("Visualization"));
	{
		AG_MenuTool(mi, tbTop, _("New scope..."), esIconScope.s,
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
