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
 * Base class for components in circuits.
 */

#include <eda.h>
#include <config/sharedir.h>

void
ES_ComponentFreePorts(ES_Component *com)
{
	int i;
	
	for (i = 0; i < com->npairs; i++) {
		ES_Pair *pair = &com->pairs[i];

		Free(pair->loops);
		Free(pair->lpols);
	}
	Free(com->pairs);
	com->pairs = NULL;
}

static void
FreeDataset(void *p)
{
	ES_Component *com = p;
	ES_SchemBlock *sb, *sbNext;

	for (sb = TAILQ_FIRST(&com->blocks);
	     sb != TAILQ_END(&com->blocks);
	     sb = sbNext) {
		sbNext = TAILQ_NEXT(sb, blocks);
		VG_NodeDetach(sb);
		VG_NodeDestroy(sb);
	}
	TAILQ_INIT(&com->blocks);
	
	ES_ComponentFreePorts(com);
}

static void
Destroy(void *p)
{
}

void
ES_ComponentSelect(ES_Component *com)
{
	com->selected = 1;
}

void
ES_ComponentUnselect(ES_Component *com)
{
	com->selected = 0;
}
		
void
ES_ComponentUnselectAll(ES_Circuit *ckt)
{
	ES_Component *com;

	CIRCUIT_FOREACH_COMPONENT(com, ckt)
		ES_ComponentUnselect(com);
}

static void
AttachSchemPorts(ES_Component *com, VG_Node *vn)
{
	VG_Node *vnChld;

	if (VG_NodeIsClass(vn, "SchemPort")) {
		ES_SchemPort *sp = (ES_SchemPort *)vn;
		ES_Port *port;

		if ((port = ES_FindPort(com, sp->name)) != NULL) {
			if (port->sp != NULL) {
				printf("%s%u: \"%s\" was already using %s%u!\n",
				    vn->ops->name, vn->handle, sp->name,
				    VGNODE(port->sp)->ops->name,
				    VGNODE(port->sp)->handle);
			}
			port->sp = sp;
			sp->com = com;
			sp->port = port;
		} else {
			printf("%s%u: No such port: \"%s\"\n",
			    vn->ops->name, vn->handle, sp->name);
			VG_NodeDetach(vn);
			return;
		}
	}
	VG_FOREACH_CHLD(vnChld, vn, vg_node)
		AttachSchemPorts(com, vnChld);
}

/*
 * Attach a schematic block to a component. Also scan the block for ES_SchemPort
 * entities, and associate them with the component's ES_Ports.
 */
void
ES_AttachSchem(ES_Component *com, ES_SchemBlock *sb)
{
	ES_SchemPort *sp;

	TAILQ_INSERT_TAIL(&com->blocks, sb, blocks);
	sb->com = com;
	AttachSchemPorts(com, VGNODE(sb));
}

/* Remove a schematic block associated with a component. */
void
ES_DetachSchem(ES_Component *com, ES_SchemBlock *sb)
{
	TAILQ_REMOVE(&com->blocks, sb, blocks);
	VG_NodeDetach(sb);
}

/* Load a component schematic block from a file. */
ES_SchemBlock *
ES_LoadSchemFromFile(ES_Component *com, const char *path)
{
	ES_SchemBlock *sb;

	sb = ES_SchemBlockNew(com->ckt->vg->root, OBJECT(com)->name);
	if (ES_SchemBlockLoad(sb, path) == -1) {
		VG_NodeDetach(sb);
		VG_NodeDestroy(sb);
		return (NULL);
	}
	return (sb);
}

static void
OnAttach(AG_Event *event)
{
	ES_Component *com = AG_SELF();
	ES_Circuit *ckt = AG_SENDER();
	
	if (!AG_ObjectIsClass(ckt, "ES_Circuit:*"))
		return;

	ES_LockCircuit(ckt);
	Debug(ckt, "Attaching component: %s\n", OBJECT(com)->name);
	Debug(com, "Attaching to circuit: %s\n", OBJECT(ckt)->name);
	com->ckt = ckt;

	if (COMOPS(com)->schemFile != NULL) {
		ES_SchemBlock *sb;
		char path[FILENAME_MAX];
	
		Strlcpy(path, SHAREDIR, sizeof(path));
		Strlcat(path, "/Schematics/", sizeof(path));
		Strlcat(path, COMOPS(com)->schemFile, sizeof(path));
		if ((sb = ES_LoadSchemFromFile(com, path)) != NULL) {
			ES_AttachSchem(com, sb);
		} else {
			AG_TextMsgFromError();
		}
	}
	if (COMOPS(com)->draw != NULL) {
		COMOPS(com)->draw(com, ckt->vg);
	}

	ES_UnlockCircuit(ckt);
}

static void
OnDetach(AG_Event *event)
{
	ES_Component *com = AG_SELF();
	ES_Circuit *ckt = AG_SENDER();
	ES_SchemBlock *sb;
	ES_Port *port;
	ES_Pair *pair;
	u_int i;

	if (!AG_ObjectIsClass(ckt, "ES_Circuit:*"))
		return;

	ES_LockCircuit(ckt);
	Debug(ckt, "Detaching component: %s\n", OBJECT(com)->name);
	Debug(com, "Detaching from circuit: %s\n", OBJECT(ckt)->name);
	AG_PostEvent(ckt, com, "circuit-disconnected", NULL);

	while ((sb = TAILQ_FIRST(&com->blocks)) != NULL) {
		ES_DetachSchem(com, sb);
#if 0
		/* XXX leak */
		VG_NodeDestroy(sb);
#endif
	}

	for (i = 0; i <= ckt->n; i++) {
		ES_Node *node = ckt->nodes[i];
		ES_Branch *br, *nbr;

		for (br = TAILQ_FIRST(&node->branches);
		     br != TAILQ_END(&node->branches);
		     br = nbr) {
			nbr = TAILQ_NEXT(br, branches);
			if (br->port != NULL && br->port->com == com)
				ES_CircuitDelBranch(ckt, i, br);
		}
	}

del_nodes:
	/* XXX hack */
	for (i = 1; i <= ckt->n; i++) {
		ES_Node *node = ckt->nodes[i];
		ES_Branch *br;

		if (node->nBranches == 0) {
			ES_CircuitDelNode(ckt, i);
			goto del_nodes;
		}
	}

	COMPONENT_FOREACH_PORT(port, i, com) {
		port->branch = NULL;
		port->node = -1;
		port->flags = 0;
		port->sp = NULL;
	}
	COMPONENT_FOREACH_PAIR(pair, i, com) {
		pair->nloops = 0;
	}
	com->ckt = NULL;

	ES_UnlockCircuit(ckt);
}

static void
Init(void *obj)
{
	ES_Component *com = obj;

	com->ckt = NULL;
	com->Tspec = 27+273.15;
	com->nports = 0;
	com->pairs = NULL;
	com->npairs = 0;
	com->specs = NULL;
	com->nspecs = 0;

	com->selected = 0;
	com->highlighted = 0;
	
	com->loadDC_G = NULL;
	com->loadDC_BCD = NULL;
	com->loadDC_RHS = NULL;
	com->loadAC = NULL;
	com->loadSP = NULL;
	com->intStep = NULL;
	com->intUpdate = NULL;
	TAILQ_INIT(&com->blocks);

	AG_SetEvent(com, "attached", OnAttach, NULL);
	AG_SetEvent(com, "detached", OnDetach, NULL);
}

/* Initialize the ports of a component instance from a given model. */
void
ES_ComponentSetPorts(void *p, const ES_Port *ports)
{
	ES_Component *com = p;
	const ES_Port *modelPort;
	ES_Pair *pair;
	ES_Port *iPort, *jPort;
	int i, j, k;

	/* Instantiate the port array. */
	ES_ComponentFreePorts(com);
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
		Debug(com, "Added port #%d (%s)\n", i, port->name);
	}

	/* Find all non-redundant port pairs. */
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
			pair->lpols = Malloc(sizeof(int));
			pair->nloops = 0;
			Debug(com, "Found pair: %s<->%s\n",
			    pair->p1->name,
			    pair->p2->name);
		}
	}
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Component *com = p;
	float Tspec;

	com->flags = (u_int)AG_ReadUint32(buf);
	com->Tspec = AG_ReadFloat(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Component *com = p;

	AG_WriteUint32(buf, com->flags);
	AG_WriteFloat(buf, com->Tspec);
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

	for (i = 0; i < pair->nloops; i++) {
		if (pair->loops[i] == loop) {
			*sign = pair->lpols[i];
			return (1);
		}
	}
	return (0);
}

void
ES_ComponentLog(void *p, const char *fmt, ...)
{
	char buf[1024];
	int len;
	ES_Component *com = p;
	AG_ConsoleLine *ln;
	va_list args;

	if (com->ckt == NULL || com->ckt->console == NULL)
		return;
	
	Strlcpy(buf, OBJECT(com)->name, sizeof(buf));
	
	va_start(args, fmt);
	ln = AG_ConsoleAppendLine(com->ckt->console, NULL);
	len = Strlcat(buf, ": ", sizeof(buf));
	vsnprintf(&buf[len], sizeof(buf)-len, fmt, args);
	ln->text = strdup(buf);
	ln->len = strlen(ln->text);
	va_end(args);
}

Uint
ES_PortNode(ES_Component *com, int port)
{
	if (port > com->nports) {
		Fatal("%s: Bad port %d/%u", OBJECT(com)->name, port,
		    com->nports);
	}
	if (com->ports[port].node < 0 || com->ports[port].node > com->ckt->n) {
		Fatal("%s:%d: Bad node", OBJECT(com)->name, port);
	}
	return (com->ports[port].node);
}

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

static void
EditParameters(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);

	/* XXX */
//	DEV_BrowserOpenData(com);
}

static void
RemoveComponent(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	ES_Circuit *ckt = com->ckt;

	ES_LockCircuit(ckt);
	AG_ObjectDetach(com);
	if (!AG_ObjectInUse(com)) {
		AG_TextTmsg(AG_MSG_INFO, 250, _("Removed component %s."),
		    OBJECT(com)->name);
		AG_ObjectDestroy(com);
	} else {
		AG_TextMsg(AG_MSG_ERROR, _("%s: Component is in use."),
		    OBJECT(com)->name);
	}
	ES_CircuitModified(ckt);
	ES_UnlockCircuit(ckt);
}

static void
RemoveSelections(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	ES_Component *com;

	ES_LockCircuit(ckt);
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (!com->selected) {
			continue;
		}
		AG_ObjectDetach(com);
		if (!AG_ObjectInUse(com)) {
			AG_ObjectDestroy(com);
		} else {
			AG_TextMsg(AG_MSG_ERROR, _("%s: Component is in use."),
			    OBJECT(com)->name);
		}
		ES_CircuitModified(ckt);
	}
	ES_UnlockCircuit(ckt);
}

static void
LoadComponent(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);

	if (AG_ObjectLoad(com) == -1)
		AG_TextMsg(AG_MSG_ERROR, "%s", AG_GetError());
}

static void
SaveComponent(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);

	if (AG_ObjectSave(com) == -1)
		AG_TextMsg(AG_MSG_ERROR, "%s", AG_GetError());
}

static void
SaveComponentTo(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);

	/* XXX */
//	DEV_BrowserSaveTo(com, _(COMOPS(com)->name));
}

static void
LoadComponentFrom(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);

	/* XXX */
//	DEV_BrowserLoadFrom(com, _(COMOPS(com)->name));
}

static void
SelectTool(AG_Event *event)
{
	VG_View *vgv = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	VG_ToolOps *ops = AG_PTR(3);
	VG_Tool *t;

	if ((t = VG_ViewFindToolByOps(vgv, ops)) != NULL) {
		VG_ViewSelectTool(vgv, t, ckt);
//		AG_WidgetFocus(vgv);
//		AG_WindowFocus(AG_WidgetParentWindow(vgv));
	} else {
		AG_TextMsg(AG_MSG_ERROR, _("No such tool: %s"), ops->name);
	}
}

void
ES_ComponentOpenMenu(ES_Component *com, VG_View *vgv)
{
	AG_PopupMenu *pm;
	Uint nsel = 0;
	ES_Component *com2;
	int common_class = 1;
	ES_ComponentClass *comCls = NULL;

	CIRCUIT_FOREACH_COMPONENT(com2, com->ckt) {
		if (!com2->selected) {
			continue;
		}
		if (comCls != NULL && comCls != COMOPS(com2)) {
			common_class = 0;
		}
		comCls = COMOPS(com2);
		nsel++;
	}

	pm = AG_PopupNew(vgv);
	{
		extern VG_ToolOps esCircuitSelectTool;
		extern VG_ToolOps esWireTool;

		AG_MenuAction(pm->item, _("Select tool"), NULL,
		    SelectTool, "%p,%p,%p", vgv, com->ckt,
		    &esSelectTool);
		AG_MenuAction(pm->item, _("Wire tool"), NULL,
		    SelectTool, "%p,%p,%p", vgv, com->ckt,
		    &esWireTool);
		AG_MenuSeparator(pm->item);
	}
	AG_MenuSection(pm->item, "[Component: %s]", OBJECT(com)->name);
	AG_MenuAction(pm->item, _("    Edit parameters"), agIconDoc.s,
	    EditParameters, "%p", com);
	AG_MenuAction(pm->item, _("    Destroy instance"), agIconTrash.s,
	    RemoveComponent, "%p", com);
	AG_MenuAction(pm->item, _("    Export model..."), agIconSave.s,
	    SaveComponentTo, "%p", com);
	AG_MenuAction(pm->item, _("    Import model..."), agIconDocImport.s,
	    LoadComponentFrom, "%p", com);
	if (COMOPS(com)->instance_menu != NULL) {
		AG_MenuSeparator(pm->item);
		COMOPS(com)->instance_menu(com, pm->item);
	}

	if (nsel > 1) {
		if (common_class && COMOPS(com)->class_menu != NULL) {
			AG_MenuSeparator(pm->item);
			AG_MenuSection(pm->item, _("[Class: %s]"),
			    comCls->name);
			COMOPS(com)->class_menu(com->ckt, pm->item);
		}
		AG_MenuSeparator(pm->item);
		AG_MenuSection(pm->item, _("[All selections]"));
		AG_MenuAction(pm->item, _("    Destroy selections"),
		    agIconTrash.s,
		    RemoveSelections, "%p", com->ckt);
	}

	AG_PopupShow(pm);
}

void
ES_UnselectAllPorts(ES_Circuit *ckt)
{
	ES_Component *com;
	ES_Port *port;
	int i;

	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		COMPONENT_FOREACH_PORT(port, i, com)
			port->flags &= ~(ES_PORT_SELECTED);
	}
}

/* Evaluate whether the given port is connected to the reference point. */
int
ES_PortIsGrounded(ES_Port *port)
{
	return (port->node == 0);
}

AG_ObjectClass esComponentClass = {
	"ES_Component",
	sizeof(ES_Component),
	{ 0,0 },
	Init,
	FreeDataset,
	Destroy,
	Load,
	Save,
	NULL,			/* edit */
};
