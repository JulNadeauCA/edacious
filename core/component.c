/*
 * Copyright (c) 2008 
 *
 * Antoine Levitt (smeuuh@gmail.com)
 * Steven Herbst (herbst@mit.edu)
 *
 * Hypertriton, Inc. <http://hypertriton.com/>
 *
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
 * Base class for components in circuits.
 */

#include "core.h"
#include <config/sharedir.h>

static void
FreeSchems(ES_Component *com)
{
	VG *vg;

	while ((vg = TAILQ_FIRST(&com->schems)) != NULL) {
		TAILQ_REMOVE(&com->schems, vg, user);
		VG_Destroy(vg);
		Free(vg);
	}
	TAILQ_INIT(&com->schems);
}

static void
FreeSchemEnts(ES_Component *com)
{
	VG_Node *vn;

	while ((vn = TAILQ_FIRST(&com->schemEnts)) != NULL) {
		VG_NodeDetach(vn);
		VG_NodeDestroy(vn);
	}
	TAILQ_INIT(&com->schemEnts);
}

static void
FreePairs(ES_Component *com)
{
	int i;

	for (i = 0; i < com->npairs; i++) {
		ES_Pair *pair = &com->pairs[i];
	
		Free(pair->loops);
		Free(pair->loopPolarity);
	}
	Free(com->pairs);
	com->pairs = NULL;
}

static void
FreeDataset(void *p)
{
	ES_Component *com = p;

	FreeSchems(com);
	FreeSchemEnts(com);
}

static void
Destroy(void *p)
{
	ES_Component *com = p;

	FreePairs(com);
}

static void
AttachSchemPorts(ES_Component *com, VG_Node *vn)
{
	VG_Node *vnChld;

	if (VG_NodeIsClass(vn, "SchemPort")) {
		ES_SchemPort *sp = (ES_SchemPort *)vn;
		ES_Port *port;

		if ((port = ES_FindPort(com, sp->name)) != NULL) {
			port->sp = sp;
			sp->com = com;
			sp->port = port;
		} else {
			VG_NodeDetach(vn);
			return;
		}
	}
	VG_FOREACH_CHLD(vnChld, vn, vg_node)
		AttachSchemPorts(com, vnChld);
}

/*
 * Associate a schematic entity with a Component. If the entity is a SchemBlock,
 * we also scan the block for SchemPorts and link them.
 */
void
ES_AttachSchemEntity(void *pCom, VG_Node *vn)
{
	ES_Component *com = pCom;

	TAILQ_INSERT_TAIL(&com->schemEnts, vn, user);
	vn->p = com;
	if (VG_NodeIsClass(vn, "SchemBlock")) {
		SCHEM_BLOCK(vn)->com = com;
	} else if (VG_NodeIsClass(vn, "SchemWire")) {
		SCHEM_WIRE(vn)->wire = WIRE(com);
	}
	AttachSchemPorts(com, vn);
}

/* Remove a schematic block associated with a component. */
void
ES_DetachSchemEntity(void *pCom, VG_Node *vn)
{
	ES_Component *com = pCom;
	vn->p = NULL;
	TAILQ_REMOVE(&com->schemEnts, vn, user);
}

/*
 * Circuit<-Component attach callback. If a schematic file is specified in
 * the Component's class information, merge its contents into the Circuit's
 * schematic. If a draw() operation exists, invoke it as well.
 */
static void
OnAttach(AG_Event *event)
{
	ES_Component *com = AG_SELF();
	ES_Circuit *ckt = AG_SENDER();
	VG *vg;
	
	if (!AG_OfClass(ckt, "ES_Circuit:*"))
		return;

	ES_LockCircuit(ckt);
	Debug(ckt, "Attach %s\n", OBJECT(com)->name);
	com->ckt = ckt;

	/*
	 * Instantiate the model schematics (schems), into a set of SchemBlock
	 * entities in the Circuit VG. For efficiency reasons, the schems list
	 * must be discarded in the process.
	 */
	TAILQ_FOREACH(vg, &com->schems, user) {
		ES_SchemBlock *sb;

		Debug(com, "Instantiating model schematic: %p\n", vg);
		sb = ES_SchemBlockNew(ckt->vg->root, OBJECT(com)->name);
		VG_Merge(sb, vg);
		ES_AttachSchemEntity(com, VGNODE(sb));
	}
	while ((vg = TAILQ_FIRST(&com->schems)) != NULL) {
		TAILQ_REMOVE(&com->schems, vg, user);
		VG_Destroy(vg);
		Free(vg);
	}
	TAILQ_INIT(&com->schems);

	/*
	 * Invoke the Component draw() operation, which may generate one or
	 * more SchemBlock entities in the Circuit VG.
	 */
	if (COMCLASS(com)->draw != NULL) {
		COMCLASS(com)->draw(com, ckt->vg);
	}
	ES_UnlockCircuit(ckt);
}

/*
 * Circuit<-Component detach callback. We update the Circuit structures and
 * remove all schematic entities related to the Component. The Component's
 * structures are reinitialized to a consistent state so that it may be reused.
 */
static void
OnDetach(AG_Event *event)
{
	ES_Component *com = AG_SELF();
	ES_Circuit *ckt = AG_SENDER();
	VG_Node *vn;
	ES_Port *port;
	ES_Pair *pair;
	Uint i;

	if (!AG_OfClass(ckt, "ES_Circuit:*"))
		return;

	ES_LockCircuit(ckt);

	/* Notify the component model. */
	Debug(ckt, "Detach %s\n", OBJECT(com)->name);
	AG_PostEvent(ckt, com, "circuit-disconnected", NULL);

	/* Remove related entities in the circuit schematic. */
	while ((vn = TAILQ_FIRST(&com->schemEnts)) != NULL) {
		Debug(com, "Removing schematic entity: %s%u\n",
		    vn->ops->name, vn->handle);
		ES_DetachSchemEntity(com, vn);
		if (VG_Delete(vn) == -1)
			AG_FatalError("Deleting node: %s", AG_GetError());
	}

del_branches:
	/* Remove the associated branches. XXX */
	for (i = 0; i < ckt->n; i++) {
		ES_Node *node = ckt->nodes[i];
		ES_Branch *br, *nbr;

		for (br = TAILQ_FIRST(&node->branches);
		     br != TAILQ_END(&node->branches);
		     br = nbr) {
			nbr = TAILQ_NEXT(br, branches);
			if (br->port != NULL && br->port->com == com) {
				Debug(com, "Removing branch in n%u\n", i);
				ES_DelBranch(ckt, i, br);
				goto del_branches;
			}
		}
	}

del_nodes:
	/* Remove node structures no longer needed. XXX */
	for (i = 1; i < ckt->n; i++) {
		ES_Node *node = ckt->nodes[i];

		if (node->nBranches == 0) {
			Debug(com, "n%u has no more branches; removing\n", i);
			ES_DelNode(ckt, i);
			goto del_nodes;
		}
	}

	/* Free computed loop information from the Pair structures. */
	COMPONENT_FOREACH_PAIR(pair, i, com) {
		Free(pair->loops);
		Free(pair->loopPolarity);
		pair->nLoops = 0;
	}

	/* Remove all references to the circuit. */
	COMPONENT_FOREACH_PORT(port, i, com) {
		port->com = NULL;
		port->node = -1;
		port->branch = NULL;
		port->flags = (port->flags & ES_PORT_SAVED_FLAGS);
		port->sp = NULL;
	}
	com->ckt = NULL;

	ES_UnlockCircuit(ckt);
}

static void
Init(void *obj)
{
	ES_Component *com = obj;

	com->flags = 0;
	com->ckt = NULL;
	com->Tspec = 27.0+273.15;
	com->nports = 0;
	com->pairs = NULL;
	com->npairs = 0;
	com->specs = NULL;
	com->nspecs = 0;

	com->dcSimBegin = NULL;
	com->dcStepBegin = NULL;
	com->dcStepIter = NULL;
	com->dcStepEnd = NULL;
	com->dcSimEnd = NULL;
	com->dcUpdateError = NULL;
	
	TAILQ_INIT(&com->schems);
	TAILQ_INIT(&com->schemEnts);

	AG_SetEvent(com, "attached", OnAttach, NULL);
	AG_SetEvent(com, "detached", OnDetach, NULL);
}

/* Initialize the Ports of a Component instance. */
void
ES_InitPorts(void *p, const ES_Port *ports)
{
	ES_Component *com = p;
	const ES_Port *modelPort;
	ES_Pair *pair;
	ES_Port *iPort, *jPort;
	int i, j, k;

	/* Instantiate the array of Ports. */
	if (com->npairs > 0) {
		FreePairs(com);
	}
	com->nports = 0;
	for (i = 1, modelPort = &ports[1];
	     i < COMPONENT_MAX_PORTS && modelPort->n >= 0;
	     i++, modelPort++) {
		ES_Port *port = &com->ports[i];

		Strlcpy(port->name, modelPort->name, sizeof(port->name));
		port->n = i;
		port->com = com;
		port->node = -1;
		port->branch = NULL;
		port->flags = 0;
		port->sp = NULL;
		com->nports++;
/*		Debug(com, "Added port #%d (%s)\n", i, port->name); */
	}

	/* Compute all non-redundant pairs of Ports. */
	com->pairs = Malloc(sizeof(ES_Pair));
	com->npairs = 0;
	COMPONENT_FOREACH_PORT(iPort, i, com) {
		COMPONENT_FOREACH_PORT(jPort, j, com) {
			if (i == j) {
				continue;
			}
			for (k = 0; k < com->npairs; k++) {
				ES_Pair *pair = &com->pairs[k];

				if (pair->p2 == iPort &&
				    pair->p1 == jPort)
					break;
			}
			if (k < com->npairs) {
				continue;
			}
			com->pairs = Realloc(com->pairs,
			    (com->npairs+1)*sizeof(ES_Pair));
			pair = &com->pairs[com->npairs++];
			pair->com = com;
			pair->p1 = iPort;
			pair->p2 = jPort;
			pair->loops = Malloc(sizeof(ES_Loop *));
			pair->loopPolarity = Malloc(sizeof(int));
			pair->nLoops = 0;
		}
	}
}

static int
Load(void *p, AG_DataSource *ds, const AG_Version *ver)
{
	ES_Component *com = p;
	Uint i, count;
	VG *vg;
	
	com->flags = (Uint)AG_ReadUint32(ds);
	com->Tspec = M_ReadReal(ds);

	/* Load the schematic blocks. */
	count = (Uint)AG_ReadUint32(ds);
	for (i = 0; i < count; i++) {
		Debug(com, "Loading schematic block %u/%u\n", i, count);
		if ((vg = VG_New(0)) == NULL) {
			return (-1);
		}
		if (VG_Load(vg, ds) == -1) {
			return (-1);		/* XXX TODO undo others */
		}
		TAILQ_INSERT_TAIL(&com->schems, vg, user);
	}
	Debug(com, "Loaded %u schematic blocks\n", count);

	return (0);
}

static int
Save(void *p, AG_DataSource *ds)
{
	ES_Component *com = p;
	off_t countOffs;
	Uint32 count;
	VG *vg;

	AG_WriteUint32(ds, com->flags & ES_COMPONENT_SAVED_FLAGS);
	M_WriteReal(ds, com->Tspec);

	/* Save the schematic blocks. */
	countOffs = AG_Tell(ds);
	count = 0;
	AG_WriteUint32(ds, 0);
	TAILQ_FOREACH(vg, &com->schems, user) {
		VG_Save(vg, ds);
		count++;
	}
	AG_WriteUint32At(ds, count, countOffs);
	
	return (0);
}

/*
 * Check whether a given pair is part of a given loop, and return
 * its polarity with respect to the loop direction.
 */
int
ES_PairIsInLoop(ES_Pair *pair, ES_Loop *loop, int *sign)
{
	unsigned int i;

	for (i = 0; i < pair->nLoops; i++) {
		if (pair->loops[i] == loop) {
			*sign = pair->loopPolarity[i];
			return (1);
		}
	}
	return (0);
}

/* Log a message to the Component's log console. */
void
ES_ComponentLog(void *p, const char *fmt, ...)
{
	char buf[1024];
	int len;
	ES_Component *com = p;
	AG_ConsoleLine *ln;
	va_list args;

	va_start(args, fmt);
#ifdef ES_DEBUG
	fprintf(stderr, "%s: ", OBJECT(com)->name);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
#endif
	if (com->ckt != NULL && com->ckt->console != NULL) {
		Strlcpy(buf, OBJECT(com)->name, sizeof(buf));
		ln = AG_ConsoleAppendLine(com->ckt->console, NULL);
		len = Strlcat(buf, ": ", sizeof(buf));
		vsnprintf(&buf[len], sizeof(buf)-len, fmt, args);
		ln->text = strdup(buf);
		ln->len = strlen(ln->text);
	}
	va_end(args);
}

/* Lookup the Circuit Node connected to the given Port by number, or die. */
Uint
ES_PortNode(ES_Component *com, int port)
{
	if (port > com->nports) {
		Fatal("%s: Bad port %d/%u", OBJECT(com)->name, port,
		    com->nports);
	}
	if (com->ports[port].node < 0 || com->ports[port].node >= com->ckt->n) {
		Fatal("%s:%d: Bad node (Component)", OBJECT(com)->name, port);
	}
	return (com->ports[port].node);
}

/* Lookup a Component Port by name. */
ES_Port *
ES_FindPort(void *p, const char *portname)
{
	ES_Component *com = p;
	ES_Port *port;
	int i;

	COMPONENT_FOREACH_PORT(port, i, com) {
		if (strcmp(port->name, portname) == 0)
			return (port);
	}
	AG_SetError(_("%s: No such port: <%s>"), OBJECT(com)->name, portname);
	return (NULL);
}

ES_ComponentClass esComponentClass = {
	{
		"Edacious(Circuit:Component)",
		sizeof(ES_Component),
		{ 0,0 },
		Init,
		FreeDataset,
		Destroy,
		Load,
		Save,
		ES_ComponentEdit
	},
	N_("Component"),
	"U",
	"Generic",
	&esIconComponent,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
