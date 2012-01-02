/*
 * Copyright (c) 2006-2009 Julien Nadeau (vedge@hypertriton.com)
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
#include <errno.h>
#include <stdio.h>
#include <config/version.h>

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
NodeVoltageFn(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	int v = AG_INT(2);

	return ES_NodeVoltage(ckt, v);
}

/* Update the branch current entries in the Circuit's property table. */
static M_Real
BranchCurrentFn(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	int i = AG_INT(2);

	return ES_BranchCurrent(ckt, i);
}

/* Commit a change in the circuit topology. */
void
ES_CircuitModified(ES_Circuit *ckt)
{
	char key[32];
	ES_Component *com;
/*	ES_Vsource *vs; */
	Uint i;

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
#if 0
	/* XXX TODO clear the v* and I* variables only */
	AG_ObjectFreeVariables(OBJECT(ckt));
#endif
	for (i = 1; i < ckt->n; i++) {
		Snprintf(key, sizeof(key), "v%u", i);
		M_BindRealFn(ckt, key, NodeVoltageFn, "%p,%i", ckt, i);
	}
	for (i = 0; i < ckt->m; i++) {
		Snprintf(key, sizeof(key), "I%u", i);
		M_BindRealFn(ckt, key, BranchCurrentFn, "%p,%i", ckt, i);
	}
	
	/* Notify the component models of the change. */
	CIRCUIT_FOREACH_COMPONENT(com, ckt)
		AG_PostEvent(ckt, com, "circuit-modified", NULL);
}

ES_Circuit *
ES_CircuitNew(void *parent, const char *name)
{
	ES_Circuit *ckt;

	ckt = Malloc(sizeof(ES_Circuit));
	AG_ObjectInit(ckt, &esCircuitClass);
	AG_ObjectSetNameS(ckt, name);
	AG_ObjectAttach(parent, ckt);
	return (ckt);
}

static void
Init(void *p)
{
	ES_Circuit *ckt = p;

	/* OBJECT(ckt)->flags |= AG_OBJECT_DEBUG_DATA; */

	ckt->flags = ES_CIRCUIT_SHOW_NODES|ES_CIRCUIT_SHOW_NODESYMS;
	ckt->descr[0] = '\0';
	ckt->authors[0] = '\0';
	ckt->keywords[0] = '\0';
	ckt->sim = NULL;
	ckt->loops = NULL;
	ckt->l = 0;
	ckt->vSrcs = NULL;
	ckt->m = 0;
	ckt->nodes = Malloc(sizeof(ES_Node *));
	ckt->console = NULL;
	ckt->extObjs = NULL;
	ckt->nExtObjs = 0;
	TAILQ_INIT(&ckt->layouts);
	TAILQ_INIT(&ckt->syms);
	TAILQ_INIT(&ckt->components);
	
	ckt->n = 1;
	InitGround(ckt);

	ckt->vg = VG_New(0);
	Strlcpy(ckt->vg->layers[0].name, _("Schematic"),
	    sizeof(ckt->vg->layers[0].name));
	
	AG_SetEvent(ckt, "edit-open", EditOpen, NULL);
	AG_SetEvent(ckt, "edit-close", EditClose, NULL);
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
	VG_Clear(ckt->vg);
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
	VG_Text *vt;

	/* Circuit information */
	AG_CopyString(ckt->descr, ds, sizeof(ckt->descr));
	AG_CopyString(ckt->authors, ds, sizeof(ckt->authors));
	AG_CopyString(ckt->keywords, ds, sizeof(ckt->keywords));
	ckt->flags = AG_ReadUint32(ds);

	/* Component models */
	count = (Uint)AG_ReadUint32(ds);
	for (i = 0; i < count; i++) {
		char comName[AG_OBJECT_NAME_MAX];
		char classSpec[AG_OBJECT_TYPE_MAX];
		AG_ObjectClass *comClass;
		Uint32 skipSize;
	
		AG_CopyString(comName, ds, sizeof(comName));
		AG_CopyString(classSpec, ds, sizeof(classSpec));
		skipSize = AG_ReadUint32(ds);
	
		Debug(ckt, "Loading component: %s (%s), %u bytes\n",
		    comName, classSpec, (Uint)skipSize);

		/*
		 * Lookup the component class. If a "@libs" specification
		 * was given, attempt to load the specified libraries.
		 */
		if ((comClass = AG_LoadClass(classSpec)) == NULL) {
			/* XXX TODO skip here? */
			return (-1);
		}

		/* Load the component instance. */
		if ((com = AG_TryMalloc(comClass->size)) == NULL) {
			return (-1);
		}
		AG_ObjectInit(com, comClass);
		AG_ObjectSetNameS(com, comName);
		if (AG_ObjectUnserialize(com, ds) == -1) {
			AG_ObjectDestroy(com);
			return (-1);
		}
		AG_ObjectAttach(ckt, com);
		TAILQ_INSERT_TAIL(&ckt->components, com, components);
		com->flags |= ES_COMPONENT_CONNECTED;
	}

	/* Circuit nodes and branches */
	count = (Uint)AG_ReadUint32(ds);
	for (i = 0; i < count; i++) {
		Uint nBranches, flags;
		int name;

		flags = (Uint)AG_ReadUint32(ds);
		nBranches = (Uint)AG_ReadUint32(ds);
		if (i != 0) {
			name = ES_AddNode(ckt);
			ckt->nodes[name]->flags = flags & ~(CKTNODE_EXAM);
		} else {
			name = 0;
		}

		Debug(ckt, "Node %u has %u branches\n", i, nBranches);
		for (j = 0; j < nBranches; j++) {
			char comName[AG_OBJECT_PATH_MAX];
			ES_Branch *br;
			Uint pPort;

			AG_CopyString(comName, ds, sizeof(comName));
			AG_ReadUint32(ds);			/* Pad */
			pPort = (Uint)AG_ReadUint32(ds);
			
			if ((com = AG_ObjectFindChild(ckt, comName)) == NULL) {
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
	if (VG_Load(ckt->vg, ds) == -1) {
		return (-1);
	}
	VG_FOREACH_NODE_CLASS(vt, ckt->vg, vg_text, "Text")
		VG_TextSubstObject(vt, ckt);

	VG_FOREACH_NODE_CLASS(sb, ckt->vg, es_schem_block, "SchemBlock") {
		if ((com = AG_ObjectFindChild(ckt, sb->name)) == NULL) {
			AG_SetError("SchemBlock: No such component: \"%s\"",
			    sb->name);
			return (-1);
		}
		sb->com = com;
	}
	VG_FOREACH_NODE_CLASS(sw, ckt->vg, es_schem_wire, "SchemWire") {
		if ((com = AG_ObjectFindChild(ckt, sw->name)) == NULL) {
			AG_SetError("SchemWire: No such wire: \"%s\"",
			    sw->name);
			return (-1);
		}
		sw->wire = com;
	}
	VG_FOREACH_NODE_CLASS(sp, ckt->vg, es_schem_port, "SchemPort") {
		if (sp->comName[0] == '\0' || sp->portName == -1) {
			continue;
		}
		if ((com = AG_ObjectFindChild(ckt, sp->comName)) == NULL) {
			AG_SetError("SchemPort refers to unexisting "
			            "component: \"%s\"", sp->name);
			return (-1);
		}
		if (sp->portName < 1 || sp->portName > com->nports) {
			AG_SetError("SchemPort refers to invalid "
			            "port of %s: %d", sp->comName,
				    sp->portName);
			return (-1);
		}
		sp->port = &com->ports[sp->portName];
	}
	
	/* Symbol table */
	count = (Uint)AG_ReadUint32(ds);
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

	/* Notify components */
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
	AG_WriteUint32(ds, 0);
	count = 0;
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		Debug(ckt, "Saving component: %s (%s@%s)\n", OBJECT(com)->name,
		    OBJECT_CLASS(com)->name, OBJECT_CLASS(com)->libs);

		AG_WriteString(ds, OBJECT(com)->name);
	
		if (OBJECT_CLASS(com)->libs[0] != '\0') {   /* Append "@libs" */
			char s[AG_OBJECT_TYPE_MAX];
			Strlcpy(s, OBJECT_CLASS(com)->hier, sizeof(s));
			Strlcat(s, "@", sizeof(s));
			Strlcat(s, OBJECT_CLASS(com)->libs, sizeof(s));
			AG_WriteString(ds, s);
		} else {
			AG_WriteString(ds, OBJECT_CLASS(com)->hier);
		}
		
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

		Debug(ckt, "Saving node n%u (0x%x)\n", i, node->flags);
		AG_WriteUint32(ds, (Uint32)node->flags);
		countOffs = AG_Tell(ds);
		count = 0;
		AG_WriteUint32(ds, 0);
		NODE_FOREACH_BRANCH(br, node) {
			if (br->port == NULL ||
			    br->port->com == NULL) {
				Debug(ckt, "Skipping node n%u branch %u (NULL)\n", i, (Uint)count);
				continue;
			}
			if (COMPONENT_IS_FLOATING(br->port->com)) {
				Debug(ckt, "Skipping node n%u branch %u (Component is floating)\n", i, (Uint)count);
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

/* Generate a text representation of the circuit. */
int
ES_CircuitExportTXT(ES_Circuit *ckt, const char *path)
{
	char buf[128];
	ES_Component *com;
	FILE *f;
	Uint i;

	if ((f = fopen(path, "w")) == NULL) {
		AG_SetError("%s: %s", path, strerror(errno));
		return (-1);
	}
	fprintf(f,
	    "# %s\n"
	    "# Generated by Edacious %s <http://edacious.org>\n"
	    "#\n", OBJECT(ckt)->name, VERSION);
	if (ckt->descr[0] != '\0') { fprintf(f, "# Description: %s\n", ckt->descr); }
	if (ckt->authors[0] != '\0') { fprintf(f, "# Author(s): %s\n", ckt->authors); }
	if (ckt->keywords[0] != '\0') { fprintf(f, "# Keywords: %s\n", ckt->keywords); }
	fprintf(f, "\n");

	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		fprintf(f, "%s %s {\n", OBJECT(com)->cls->hier,
		    OBJECT(com)->name);
		for (i = 0; i < OBJECT(com)->nVars; i++) {
			AG_Variable *V = &OBJECT(com)->vars[i];

			AG_PrintVariable(buf, sizeof(buf), V);
			fprintf(f, "\t%s %s = %s\n",
			    agVariableTypes[V->type].name,
			    V->name, buf);
		}
		fprintf(f, "}\n");
	}

	for (i = 0; i < ckt->n; i++) {
		ES_Node *node = ckt->nodes[i];
		ES_Branch *br;

		fprintf(f, "node %d {\n", i);
		NODE_FOREACH_BRANCH(br, node) {
			if (br->port == NULL || br->port->com == NULL ||
			    COMPONENT_IS_FLOATING(br->port->com)) {
				continue;
			}
			fprintf(f, "\t%s:%u", OBJECT(br->port->com)->name,
			    (Uint)br->port->n);
			if (br->port->name[0] != '\0') {
				fprintf(f, "\t# (%s)", br->port->name);
			}
			fprintf(f, "\n");
		}
		fprintf(f, "}\n");
	}

	fclose(f);

	AG_TextTmsg(AG_MSG_INFO, 1250,
	    _("`%s' was successfully exported to `%s'"),
	    OBJECT(ckt)->name, path);
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
#ifdef ES_DEBUG
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
#ifdef ES_DEBUG
	if (!AG_OfClass(obj, "ES_Circuit:ES_Component:*"))
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

	if (v == -1 || v <= ckt->m) {
		return;
	}
	if (v < --ckt->m) {
		for (; v < ckt->m; v++)
			ckt->vSrcs[v] = ckt->vSrcs[v+1];
	}
	Debug(ckt, "Removed voltage source v%d\n", v);
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

M_Real
ES_NodeVoltagePrevStep(ES_Circuit *ckt, int j, int n)
{
	if (ckt->sim == NULL || ckt->sim->ops->node_voltage_prev_step == NULL) {
		return (0.0);
	}
	return (ckt->sim->ops->node_voltage_prev_step(ckt->sim, j, n));
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

M_Real
ES_BranchCurrentPrevStep(ES_Circuit *ckt, int k, int n)
{
	if (ckt->sim == NULL || ckt->sim->ops->branch_current_prev_step == NULL) {
		return (0.0);
	}
	return (ckt->sim->ops->branch_current_prev_step(ckt->sim, k, n));
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
#ifdef ES_DEBUG
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

	Free(ckt->extObjs);
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
	if (agGUI &&
	    sim->ops->edit != NULL &&
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
#ifdef ES_DEBUG
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

/* Attach an external simulation object to the circuit. */
void
ES_AddSimulationObj(ES_Circuit *ckt, void *obj)
{
	ckt->extObjs = Realloc(ckt->extObjs, (ckt->nExtObjs+1) * 
	                                     sizeof(AG_Object *));
	ckt->extObjs[ckt->nExtObjs++] = obj;
	AG_ObjectAddDep(ckt, obj, 0);
}

AG_ObjectClass esCircuitClass = {
	"Edacious(Circuit)",
	sizeof(ES_Circuit),
	{ 1,0 },
	Init,
	FreeDataset,
	Destroy,
	Load,
	Save,
	ES_CircuitEdit
};
