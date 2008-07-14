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

#include "core.h"

/* Circuit edition tools */
static VG_ToolOps *ToolsCkt[] = {
	&esSchemSelectTool,
	&esWireTool,
	NULL
};

/* Circuit schematic edition tools */
static VG_ToolOps *ToolsSchem[] = {
	&vgPointTool,
	&vgLineTool,
	&vgCircleTool,
	&vgTextTool,
#ifdef DEBUG
	&vgProximityTool,
#endif
	NULL
};

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

	CIRCUIT_FOREACH_COMPONENT(com, ckt)
		AG_PostEvent(ckt, com, "circuit-shown", NULL);
}

static void
EditClose(AG_Event *event)
{
	ES_Circuit *ckt = AG_SELF();
	ES_Component *com;

	ES_DestroySimulation(ckt);
	CIRCUIT_FOREACH_COMPONENT(com, ckt)
		AG_PostEvent(ckt, com, "circuit-hidden", NULL);
}

/* Update the voltage entries in the Circuit's property table. */
static M_Real
ReadNodeVoltage(void *p, AG_Prop *pr)
{
	return (ES_NodeVoltage(p, atoi(&pr->key[1])));
}

/* Update the branch current entries in the Circuit's property table. */
static M_Real
ReadBranchCurrent(void *p, AG_Prop *pr)
{
	return (ES_BranchCurrent(p, atoi(&pr->key[1])));
}

/*
 * Effect a change in the circuit topology. We recalculate loops and invoke
 * the cktmod() operation of the current simulation object if any. The
 * circuit property table is also updated and components are notified via
 * the generic 'circuit-modified' event.
 */
void
ES_CircuitModified(ES_Circuit *ckt)
{
	char key[32];
	ES_Component *com;
/*	ES_Vsource *vs; */
	ES_Loop *loop;
	Uint i;
	AG_Prop *pr;

#if 0
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
#endif

	/* Notify the simulation object of the change. */
	if (ckt->sim != NULL &&
	    ckt->sim->ops->cktmod != NULL)
		ckt->sim->ops->cktmod(ckt->sim, ckt);

	/* Update the voltage/current entries in the property table. */
	AG_ObjectFreeProps(OBJECT(ckt));
	for (i = 0; i < ckt->n; i++) {
		Snprintf(key, sizeof(key), "v%u", i);
		pr = M_SetReal(ckt, key, 0.0);
		M_SetRealRdFn(pr, ReadNodeVoltage);
	}
	for (i = 0; i < ckt->m; i++) {
		Snprintf(key, sizeof(key), "I%u", i);
		pr = M_SetReal(ckt, key, 0.0);
		M_SetRealRdFn(pr, ReadBranchCurrent);
	}
	
	/* Notify the component models of the change. */
	CIRCUIT_FOREACH_COMPONENT(com, ckt)
		AG_PostEvent(ckt, com, "circuit-modified", NULL);
}

/*
 * Notify the child objects of the simulation progress (independent of the
 * simulation type).
 */
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

	ckt->flags = ES_CIRCUIT_SHOW_NODES|ES_CIRCUIT_SHOW_NODESYMS|
	             ES_CIRCUIT_SHOW_NODENAMES;
	ckt->descr[0] = '\0';
	ckt->authors[0] = '\0';
	ckt->keywords[0] = '\0';
	ckt->sim = NULL;
	ckt->loops = NULL;
	ckt->l = 0;
	ckt->vSrcs = NULL;
	ckt->m = 0;
	ckt->nodes = Malloc(sizeof(ES_Node *));
	ckt->n = 1;
	ckt->console = NULL;
	TAILQ_INIT(&ckt->syms);
	InitGround(ckt);

	ckt->vg = VG_New(0);
	Strlcpy(ckt->vg->layers[0].name, _("Schematic"),
	    sizeof(ckt->vg->layers[0].name));
	
	AG_SetEvent(ckt, "edit-open", EditOpen, NULL);
	AG_SetEvent(ckt, "edit-close", EditClose, NULL);

	/* Notify child objects of simulation progress. */
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

	Free(ckt->vSrcs);
	ckt->vSrcs = NULL;
	ckt->m = 0;

	for (sym = TAILQ_FIRST(&ckt->syms);
	     sym != TAILQ_END(&ckt->syms);
	     sym = nsym) {
		nsym = TAILQ_NEXT(sym, syms);
		Free(sym);
	}
	TAILQ_INIT(&ckt->syms);

	for (i = 0; i < ckt->n; i++) {
		FreeNode(ckt->nodes[i]);
	}
	ckt->nodes = Realloc(ckt->nodes, sizeof(ES_Node *));
	ckt->n = 1;
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
	Uint i, j, count;
	ES_SchemBlock *sb;
	ES_SchemWire *sw;
	ES_SchemPort *sp;

	/* Circuit information */
	AG_CopyString(ckt->descr, ds, sizeof(ckt->descr));
	AG_CopyString(ckt->authors, ds, sizeof(ckt->authors));
	AG_CopyString(ckt->keywords, ds, sizeof(ckt->keywords));
	ckt->flags = AG_ReadUint32(ds);
	
	/* Component models */
	count = (Uint)AG_ReadUint32(ds);
	Debug(ckt, "Loading components (%u)\n", count);
	for (i = 0; i < count; i++) {
		char comName[AG_OBJECT_NAME_MAX];
		char clsID[AG_OBJECT_TYPE_MAX];
		AG_ObjectClass *cls;
		Uint32 skipSize;
	
		AG_CopyString(comName, ds, sizeof(comName));
		AG_CopyString(clsID, ds, sizeof(clsID));
		skipSize = AG_ReadUint32(ds);
		
		Debug(ckt, "Loading component: %s (%s), %u bytes\n",
		    comName, clsID, (Uint)skipSize);

		if ((cls = AG_FindClass(clsID)) == NULL) {
			/* TODO skip? */
			AG_SetError("%s: Unimplemented class \"%s\"",
			    comName, clsID);
			return (-1);
		}

		com = Malloc(cls->size);
		AG_ObjectInit(com, cls);
		AG_ObjectSetName(com, "%s", comName);
		if (AG_ObjectUnserialize(com, ds) == -1) {
			AG_ObjectDestroy(com);
			return (-1);
		}
		OBJECT(com)->flags |= AG_OBJECT_RESIDENT;
		AG_ObjectAttach(ckt, com);
	}

	/* Circuit nodes and branches */
	count = (Uint)AG_ReadUint32(ds);
	Debug(ckt, "Loading nodes (%u)\n", count);
	for (i = 0; i < count; i++) {
		Uint nBranches, flags;
		int name;

		flags = (Uint)AG_ReadUint32(ds);
		nBranches = (Uint)AG_ReadUint32(ds);
		if (i == 0) {
			name = 0;
			FreeNode(ckt->nodes[0]);
			InitGround(ckt);
		} else {
			name = ES_AddNode(ckt);
			ckt->nodes[name]->flags = flags & ~(CKTNODE_EXAM);
		}

		for (j = 0; j < nBranches; j++) {
			char comName[AG_OBJECT_PATH_MAX];
			ES_Branch *br;
			Uint pPort;

			AG_CopyString(comName, ds, sizeof(comName));
			AG_ReadUint32(ds);			/* Pad */
			pPort = (Uint)AG_ReadUint32(ds);
			Debug(ckt, "n%u: Branch to %s:%u\n", i, comName,
			    pPort);
			
			if ((com = AG_ObjectFindChild(ckt, comName)) == NULL) {
				AG_SetError("%s", AG_GetError());
				return (-1);
			}
			if (pPort > com->nports) {
				AG_SetError("Bogus branch: %s:%u", comName,
				    pPort);
				return (-1);
			}
			br = ES_AddBranch(ckt, name, &com->ports[pPort]);
			com->ports[pPort].node = name;
			com->ports[pPort].branch = br;
		}
	}

	/*
	 * Load the circuit schematics and re-attach the schematic entities
	 * with the actual circuit structures.
	 */
	Debug(ckt, "Loading schematic\n");
	if (VG_Load(ckt->vg, ds) == -1) {
		return (-1);
	}
	VG_FOREACH_NODE_CLASS(sb, ckt->vg, es_schem_block, "SchemBlock") {
		if ((com = AG_ObjectFindChild(ckt, sb->name)) != NULL) {
			sb->com = com;
		} else {
			AG_SetError("SchemBlock refers to unexisting "
			            "component: \"%s\"", sb->name);
			return (-1);
		}
	}
	VG_FOREACH_NODE_CLASS(sw, ckt->vg, es_schem_wire, "SchemWire") {
		if ((com = AG_ObjectFindChild(ckt, sw->name)) != NULL) {
			sw->wire = com;
		} else {
			AG_SetError("SchemWire refers to unexisting "
			            "wire: \"%s\"", sw->name);
			return (-1);
		}
	}
	VG_FOREACH_NODE_CLASS(sp, ckt->vg, es_schem_port, "SchemPort") {
		if (sp->comName[0] == '\0' || sp->portName == -1) {
			continue;
		}
		if ((com = AG_ObjectFindChild(ckt, sp->comName)) != NULL) {
			sp->com = com;
		} else {
			AG_SetError("SchemPort refers to unexisting "
			            "component: \"%s\"", sp->name);
			return (-1);
		}
		if (sp->portName < 1 || sp->portName > sp->com->nports) {
			AG_SetError("SchemPort refers to invalid "
			            "port of %s: %d", sp->comName,
				    sp->portName);
			return (-1);
		}
		sp->port = &sp->com->ports[sp->portName];
	}

	/* Symbol table */
	count = (Uint)AG_ReadUint32(ds);
	Debug(ckt, "Loading symbol table (%u)\n", count);
	for (i = 0; i < count; i++) {
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

	/* Component port assignments */
	count = (Uint)AG_ReadUint32(ds);
	Debug(ckt, "Loading component ports (%u)\n", count);
	for (i = 0; i < count; i++) {
		char comName[AG_OBJECT_NAME_MAX];
		Uint j;
		ES_Component *com;
		ES_Port *port;

		AG_CopyString(comName, ds, sizeof(comName));
		CIRCUIT_FOREACH_COMPONENT(com, ckt) {
			if (strcmp(OBJECT(com)->name, comName) == 0)
				break;
		}
		if (com == NULL) {
			AG_SetError("Bogus port component: %s", comName);
			return (-1);
		}
		
		/* Load the port information. */
		com->nports = (Uint)AG_ReadUint32(ds);
		Debug(ckt, "Loading ports of %s (%u)\n", comName, com->nports);
		COMPONENT_FOREACH_PORT(port, j, com) {
			ES_Branch *br;
			ES_Node *node;
			int portNode;

			port->n = (int)AG_ReadUint32(ds);
			AG_CopyString(port->name, ds, sizeof(port->name));
			portNode = (int)AG_ReadUint32(ds);
			if (portNode >= ckt->n) {
				AG_SetError("Bogus port node#: %d", port->node);
				return (-1);
			}
			port->node = portNode;
			node = ckt->nodes[port->node];
		
			Debug(ckt, "Port %s:%d: name=\"%s\", node=%d\n",
			    comName, j, port->name, port->node);

			NODE_FOREACH_BRANCH(br, node) {
				if (br->port->com == com &&
				    br->port->n == port->n)
					break;
			}
			if (br == NULL) {
				AG_SetError("Port is missing branch: %u(%s)-%d",
				    port->n, port->name, port->node);
				return (-1);
			}
			port->branch = br;
			port->flags = 0;
		}
	}

	/* Send circuit-connected event to components. */
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		AG_PostEvent(ckt, com, "circuit-connected", NULL);
		AG_PostEvent(ckt, com, "circuit-shown", NULL);
	}
	ES_CircuitModified(ckt);
	return (0);
}

static int
Save(void *p, AG_DataSource *ds)
{
	ES_Circuit *ckt = p;
	ES_Branch *br;
	ES_Component *com;
	ES_Sym *sym;
	off_t countOffs, skipSizeOffs;
	Uint32 count;
	Uint i;

	/* Circuit information */
	AG_WriteString(ds, ckt->descr);
	AG_WriteString(ds, ckt->authors);
	AG_WriteString(ds, ckt->keywords);
	AG_WriteUint32(ds, ckt->flags);

	/* Component models */
	countOffs = AG_Tell(ds);
	count = 0;
	AG_WriteUint32(ds, 0);
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		printf("Saving component: %s (%s)\n", OBJECT(com)->name,
		    OBJECT_CLASS(com)->name);
		AG_WriteString(ds, OBJECT(com)->name);
		AG_WriteString(ds, OBJECT_CLASS(com)->name);
		skipSizeOffs = AG_Tell(ds);
		AG_WriteUint32(ds, 0);

		if (AG_ObjectSerialize(com, ds) == -1) {
			return (-1);
		}
		AG_WriteUint32At(ds, AG_Tell(ds)-skipSizeOffs, skipSizeOffs);
		count++;
	}
	AG_WriteUint32At(ds, count, countOffs);

	/* Circuit nodes and branches */
	AG_WriteUint32(ds, ckt->n);
	for (i = 0; i < ckt->n; i++) {
		ES_Node *node = ckt->nodes[i];

		AG_WriteUint32(ds, (Uint32)node->flags);
		countOffs = AG_Tell(ds);
		count = 0;
		AG_WriteUint32(ds, 0);
		NODE_FOREACH_BRANCH(br, node) {
			if (br->port == NULL || br->port->com == NULL ||
			    COMPONENT_IS_FLOATING(br->port->com)) {
				continue;
			}
			AG_WriteString(ds, OBJECT(br->port->com)->name);
			AG_WriteUint32(ds, 0);			/* Pad */
			AG_WriteUint32(ds, (Uint32)br->port->n);
			count++;
		}
		AG_WriteUint32At(ds, count, countOffs);
	}
	
	/* Circuit schematics */
	VG_Save(ckt->vg, ds);

	/* Symbol table */
	countOffs = AG_Tell(ds);
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
	AG_WriteUint32At(ds, count, countOffs);

	/* Component port assignments */
	countOffs = AG_Tell(ds);
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
	AG_WriteUint32At(ds, count, countOffs);
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
	ckt->nodes = Realloc(ckt->nodes, (ckt->n+1)*sizeof(ES_Node *));
	ckt->nodes[ckt->n] = Malloc(sizeof(ES_Node));
	InitNode(ckt->nodes[ckt->n]);
	Debug(ckt, "Added node n%d\n", ckt->n);
	return (ckt->n++);
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

	Debug(ckt, _("Deleting node n%d\n"), n);
#ifdef DEBUG
	if (n == 0 || n >= ckt->n)
		Fatal("Cannot delete n%d (max %u)", n, ckt->n);
#endif
	if (n != ckt->n-1) {
		/* Update the Branch port pointers. */
		for (i = n; i < ckt->n; i++) {
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
	Debug(ckt, _("Merging nodes: n%d,n%d -> n%d\n"), N1, N2, Nnew);
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

/*
 * Create a new voltage source and return the new index. Any component
 * can register as a voltage source.
 */
int
ES_AddVoltageSource(ES_Circuit *ckt, void *obj)
{
#ifdef DEBUG
	if (!AG_ObjectIsClass(obj, "ES_Component:*"))
		AG_FatalError("Not a component");
#endif
	ckt->vSrcs = Realloc(ckt->vSrcs, (ckt->m+1)*sizeof(ES_Component *));
	ckt->vSrcs[ckt->m] = obj;
	Debug(ckt, "Added voltage source v%d\n", ckt->m);
	return (ckt->m++);
}

/*
 * Remove a voltage source and all references to it. If necessary, shift the
 * entire vSrcs array and update all loop references accordingly.
 */
void
ES_DelVoltageSource(ES_Circuit *ckt, int vName)
{
	int v = vName;

	if (v == -1)
		return;
#ifdef DEBUG
	if (v <= ckt->m)
		AG_FatalError("No such voltage source: v%d", vName);
#endif
	if (v < --ckt->m) {
		for (; v < ckt->m; v++)
			ckt->vSrcs[v] = ckt->vSrcs[v+1];
	}
	Debug(ckt, "Removed voltage source v%d\n", v);
}

/* Clear the voltage sources array. */
void
ES_ClearVoltageSources(ES_Circuit *ckt)
{
	Debug(ckt, "Clearing %u voltage sources\n", ckt->m);
	Free(ckt->vSrcs);
	ckt->vSrcs = NULL;
	ckt->m = 0;
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

/* Return the DC voltage for the node j from the last simulation step. */
M_Real
ES_NodeVoltage(ES_Circuit *ckt, int j)
{
	if (ckt->sim == NULL || ckt->sim->ops->node_voltage == NULL) {
		return (0.0);
	}
	return (ckt->sim->ops->node_voltage(ckt->sim, j));
}

/* Return the branch current for the voltage source k from the last step. */
M_Real
ES_BranchCurrent(ES_Circuit *ckt, int k)
{
	if (ckt->sim == NULL || ckt->sim->ops->branch_current == NULL) {
		return (0.0);
	}
	return (ckt->sim->ops->branch_current(ckt->sim, k));
}

/* Lookup an existing node by number or die. */
ES_Node *
ES_GetNode(ES_Circuit *ckt, int n)
{
	if (n < 0 || n >= ckt->n) {
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
		Strlcpy(dst, "(null)", dst_len);
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
		    sym->p.node >= 0 && sym->p.node < ckt->n)
			return (ckt->nodes[sym->p.node]);
	}
	if (name[0] == 'n' && name[1] != '\0') {
		int n = atoi(&name[1]);

		if (n >= 0 && n < ckt->n) {
			return (ckt->nodes[n]);
		}
	}
	AG_SetError(_("No such node: `%s'"), name);
	return (NULL);
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
		char txt[32];
		ES_Loop *loop = ckt->loops[i];
		ES_Component *vSrc = loop->origin;
		AG_TlistItem *it;

		it = AG_TlistAdd(tl, NULL, "%s:L%u", OBJECT(vSrc)->name,
		    loop->name);
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
	for (i = 0; i < ckt->n; i++) {
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
	AG_TlistItem *it;
	int i;

	AG_TlistClear(tl);
	for (i = 0; i < ckt->m; i++) {
		it = AG_TlistAdd(tl, NULL, "%s", OBJECT(ckt->vSrcs[i])->name);
		it->p1 = ckt->vSrcs[i];
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
		    _("%u nodes, %u vsources, %u loops"),
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
MouseButtonDown(AG_Event *event)
{
	VG_View *vv =  AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	int button = AG_INT(2);
	float x = AG_FLOAT(3);
	float y = AG_FLOAT(4);
	VG_Node *vn;

	if (button != SDL_BUTTON_RIGHT) {
		return;
	}
	if ((vn = ES_SchemNearest(vv, VGVECTOR(x,y))) != NULL) {
		if (VG_NodeIsClass(vn, "SchemBlock")) {
			ES_SchemBlock *sb = (ES_SchemBlock *)vn;
			ES_SelectComponent(sb->com, vv);
			ES_ComponentMenu(sb->com, vv);
		} else if (VG_NodeIsClass(vn, "SchemWire")) {
			ES_SchemWire *sw = (ES_SchemWire *)vn;
			ES_SelectComponent(sw->wire, vv);
			ES_ComponentMenu(sw->wire, vv);
		}
	}
}

static void
KeyDown(AG_Event *event)
{
	VG_View *vv =  AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
}

static void
SelectSimulation(AG_Event *event)
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
FindObjects(AG_Tlist *tl, AG_Object *pob, int depth, void *ckt)
{
	AG_Object *cob;
	AG_TlistItem *it;
	int selected = 0;

	if (AG_ObjectIsClass(pob, "ES_Component:*")) {
		if (ESCOMPONENT(pob)->flags & ES_COMPONENT_SUPPRESSED) {
			it = AG_TlistAdd(tl, esIconComponent.s,
			    _("%s (suppressed)"),
			    pob->name);
		} else {
			it = AG_TlistAdd(tl, esIconComponent.s,
			    "%s", pob->name);
		}
		it->selected = (ESCOMPONENT(pob)->flags &
		                ES_COMPONENT_SELECTED);
	} else {
		it = AG_TlistAdd(tl, NULL, "%s", pob->name);
	}

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
			FindObjects(tl, cob, depth+1, ckt);
	}
}

static void
PollObjects(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	AG_Object *ckt = AG_PTR(1);
	AG_TlistItem *it;

	AG_TlistClear(tl);
	AG_LockVFS(ckt);
	FindObjects(tl, ckt, 0, ckt);
	AG_UnlockVFS(ckt);
	AG_TlistRestore(tl);
}

static void
SelectedObject(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	VG_View *vv = AG_PTR(1);
	AG_TlistItem *it = AG_PTR(2);
	int state = AG_INT(3);
	AG_Object *obj = it->p1;

	if (AG_ObjectIsClass(obj, "ES_Component:*")) {
		if (state) {
			ES_SelectComponent(COMPONENT(obj), vv);
		} else {
			ES_UnselectComponent(COMPONENT(obj), vv);
		}
	}
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
	ES_OpenObject(scope);
}

static void
InsertComponent(AG_Event *event)
{
	char name[AG_OBJECT_NAME_MAX];
	VG_View *vv = AG_PTR(1);
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
	OBJECT(com)->flags |= AG_OBJECT_RESIDENT;
	com->flags |= ES_COMPONENT_FLOATING;

	ES_LockCircuit(ckt);

	AG_ObjectAttach(ckt, com);
	AG_PostEvent(ckt, com, "circuit-shown", NULL);

	if ((t = VG_ViewFindTool(vv, "Insert component")) != NULL) {
		VG_ViewSelectTool(vv, t, ckt);
		esFloatingCom = com;
		ES_SelectComponent(esFloatingCom, vv);
	}
	ES_UnlockCircuit(ckt);
}

static void *
Edit(void *p)
{
	ES_Circuit *ckt = p;
	AG_Window *win;
	AG_Toolbar *tbTop, *tbRight;
	AG_Menu *menu;
	AG_MenuItem *mi, *mi2;
	AG_Pane *hPane, *vPane;
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
		AG_MenuAction(mi, _("Properties..."), agIconGear.s,
		    ShowProperties, "%p,%p,%p", win, ckt, vv);
		AG_MenuActionKb(mi, _("Close document"), agIconClose.s,
		    SDLK_w, KMOD_CTRL,
		    AGWINCLOSE(win));
	}

	mi = AG_MenuAddItem(menu, _("Edit"));
	{
		mi2 = AG_MenuNode(mi, _("Snapping mode"), NULL);
		VG_SnapMenu(menu, mi2, vv);
	}
	mi = AG_MenuAddItem(menu, _("View"));
	{
		AG_MenuActionKb(mi, _("New view..."), esIconCircuit.s,
		    SDLK_v, KMOD_CTRL,
		    CreateView, "%p,%p", win, ckt);
		AG_MenuAction(mi, _("Circuit topology..."), esIconMesh.s,
		    ShowTopology, "%p,%p", win, ckt);
		AG_MenuAction(mi, _("Log console..."), esIconConsole.s,
		    ShowConsole, "%p,%p", win, ckt);

		AG_MenuSeparator(mi);

		mi2 = AG_MenuNode(mi, _("Schematic"), esIconCircuit.s);
		{
			AG_MenuToolbar(mi2, tbTop);
			AG_MenuFlags(mi2, _("Nodes annotations"),
			    esIconShowNodes.s,
			    &ckt->flags, ES_CIRCUIT_SHOW_NODES, 0);
			AG_MenuFlags(mi2, _("Node names/numbers"),
			    esIconShowNodeNames.s,
			    &ckt->flags, ES_CIRCUIT_SHOW_NODENAMES, 0);
			AG_MenuToolbar(mi2, NULL);
			AG_ToolbarSeparator(tbTop);

			AG_MenuSeparator(mi2);

			AG_MenuFlags(mi2, _("Grid"), vgIconSnapGrid.s,
			    &vv->flags, VG_VIEW_GRID, 0);
			AG_MenuFlags(mi2, _("Construction geometry"),
			    esIconConstructionGeometry.s,
			    &vv->flags, VG_VIEW_CONSTRUCTION, 0);
#ifdef DEBUG
			AG_MenuFlags(mi2, _("Extents"), vgIconBlock.s,
			    &vv->flags, VG_VIEW_EXTENTS, 0);
#endif
		}
	}
	
	mi = AG_MenuAddItem(menu, _("Simulation"));
	{
		extern const ES_SimOps *esSimOps[];
		const ES_SimOps **pOps, *ops;

		AG_MenuToolbar(mi, tbTop);
		for (pOps = &esSimOps[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			AG_MenuAction(mi, _(ops->name),
			    ops->icon ? ops->icon->s : NULL,
			    SelectSimulation, "%p,%p,%p", ckt, ops, win);
		}
		AG_MenuToolbar(mi, NULL);
		AG_ToolbarSeparator(tbTop);
	}
	
	hPane = AG_PaneNewHoriz(win, AG_PANE_EXPAND);
	{
		AG_Notebook *nb;
		AG_NotebookTab *ntab;
		AG_Tlist *tl;
		AG_Box *box, *box2;

		vPane = AG_PaneNewVert(hPane->div[0], AG_PANE_EXPAND);
		nb = AG_NotebookNew(vPane->div[0], AG_NOTEBOOK_EXPAND);
		ntab = AG_NotebookAddTab(nb, _("Models"), AG_BOX_VERT);
		{
			int i;

			tl = AG_TlistNew(ntab, AG_TLIST_EXPAND);
			AG_TlistSizeHint(tl, _("Voltage source (square)"), 20);
			AG_WidgetSetFocusable(tl, 0);
			AG_SetEvent(tl, "tlist-dblclick", InsertComponent,
			    "%p,%p,%p", vv, tl, ckt);

			for (i = 0; i < esComponentClassCount; i++) {
				ES_ComponentClass *cls = esComponentClasses[i];
				AG_TlistAddPtr(tl,
				    cls->icon != NULL ? cls->icon->s : NULL,
				    cls->name, cls);
			}
		}
		ntab = AG_NotebookAddTab(nb, _("Objects"), AG_BOX_VERT);
		{
			tl = AG_TlistNew(ntab, AG_TLIST_POLL|AG_TLIST_TREE|
			                       AG_TLIST_EXPAND|
					       AG_TLIST_NOSELSTATE);
			AG_SetEvent(tl, "tlist-poll", PollObjects, "%p", ckt);
			AG_SetEvent(tl, "tlist-changed", SelectedObject,
			    "%p", vv);
			AG_WidgetSetFocusable(tl, 0);
		}
		VG_AddEditArea(vv, vPane->div[1]);

		box = AG_BoxNewVert(hPane->div[1], AG_BOX_EXPAND);
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

	mi = AG_MenuAddItem(menu, _("Tools"));
	{
		AG_MenuItem *mAction;
		VG_ToolOps **pOps, *ops;
		VG_Tool *tool;

		AG_MenuToolbar(mi, tbRight);

		for (pOps = &ToolsCkt[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			tool = VG_ViewRegTool(vv, ops, ckt);
			mAction = AG_MenuAction(mi, ops->name,
			    ops->icon ? ops->icon->s : NULL,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, ckt);
			AG_MenuSetIntBoolMp(mAction, &tool->selected, 0,
			    &OBJECT(vv)->lock);
			if (ops == &esSchemSelectTool)
				VG_ViewSetDefaultTool(vv, tool);
		}

		AG_MenuSeparator(mi);
		
		for (pOps = &ToolsSchem[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			tool = VG_ViewRegTool(vv, ops, ckt);
			mAction = AG_MenuAction(mi, ops->name,
			    ops->icon ? ops->icon->s : NULL,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, ckt);
			AG_MenuSetIntBoolMp(mAction, &tool->selected, 0,
			    &OBJECT(vv)->lock);
		}
		
		VG_ViewRegTool(vv, &esInsertTool, ckt);
		VG_ViewButtondownFn(vv, MouseButtonDown, "%p", ckt);
		VG_ViewKeydownFn(vv, KeyDown, "%p", ckt);
		
		AG_MenuToolbar(mi, NULL);
	}
	
	mi = AG_MenuAddItem(menu, _("Visualization"));
	{
		AG_MenuToolbar(mi, tbTop);
		AG_MenuAction(mi, _("New scope..."), esIconScope.s,
		    NewScope, "%p", ckt);
		AG_MenuToolbar(mi, NULL);
	}

	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 85, 85);
	AG_PaneMoveDivider(vPane, 40*agView->h/100);
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
