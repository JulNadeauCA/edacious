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

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>
#include <agar/dev.h>

#include <string.h>

#include "eda.h"

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
	ES_ComponentFreePorts(p);
}

static void
Destroy(void *p)
{
	ES_ComponentFreePorts(p);
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

	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		ES_ComponentUnselect(com);
	}
}

static void
OnRename(AG_Event *event)
{
	ES_Component *com = AG_SELF();

	if (com->block == NULL)
		return;

	AG_ObjectCopyName(com, com->block->name, sizeof(com->block->name));
}

static void
OnAttach(AG_Event *event)
{
	char blkName[VG_BLOCK_NAME_MAX];
	ES_Component *com = AG_SELF();
	ES_Circuit *ckt = AG_SENDER();
	VG *vg = ckt->vg;
	
	if (!AG_ObjectIsClass(ckt, "ES_Circuit:*")) {
		return;
	}
	Debug(ckt, "Attaching component: %s\n", OBJECT(com)->name);
	Debug(com, "Attaching to circuit: %s\n", OBJECT(ckt)->name);

	com->ckt = ckt;
	
	AG_ObjectCopyName(com, blkName, sizeof(blkName));
#ifdef DEBUG
	if (com->block != NULL) { Fatal("Block exists"); }
	if (VG_GetBlock(vg, blkName) != NULL) { Fatal("Duplicate block"); }
#endif
	com->block = VG_BeginBlock(vg, blkName, 0);
	com->block->pos.x = com->ports[0].x;
	com->block->pos.y = com->ports[0].y;
/*	VG_Origin(vg, 0, com->ports[0].x, com->ports[0].y); */
	VG_EndBlock(vg);
}

static void
OnDetach(AG_Event *event)
{
	ES_Component *com = AG_SELF();
	ES_Circuit *ckt = AG_SENDER();
	u_int i;

	if (!AG_ObjectIsClass(ckt, "ES_Circuit:*"))
		return;

	ES_LockCircuit(ckt);
	Debug(ckt, "Detaching component: %s\n", OBJECT(com)->name);
	Debug(com, "Detaching from circuit: %s\n", OBJECT(ckt)->name);
	AG_PostEvent(ckt, com, "circuit-disconnected", NULL);

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

		if (node->nbranches == 0) {
			ES_CircuitDelNode(ckt, i);
			goto del_nodes;
		}
	}
	
	for (i = 1; i <= com->nports; i++) {
		ES_Port *port = &com->ports[i];

		port->branch = NULL;
		port->node = -1;
		port->flags = 0;
	}

	for (i = 0; i < com->npairs; i++)
		com->pairs[i].nloops = 0;

	if (com->block != NULL) {
		VG_DestroyBlock(ckt->vg, com->block);
		com->block = NULL;
	}
	com->ckt = NULL;
	
	ES_UnlockCircuit(ckt);
}

/* Redraw a component schematic block. */
void
ES_ComponentDraw(ES_Component *com, VG_View *vv)
{
	ES_Circuit *ckt = com->ckt;
	VG *vg = vv->vg;
	VG_Node *vn;
	int i;

	/* Invoke the component-specific draw operation. */
	if (COMOPS(com)->draw != NULL)
		COMOPS(com)->draw(com, vg);

	/* Indicate the nodes associated with connection points. */
	for (i = 1; i <= com->nports; i++) {
		ES_Port *port = &com->ports[i];
		float r, theta;
		float x, y;

		r = hypotf(port->x,port->y);
		theta = atan2f(port->y,port->x) - com->block->theta;
		ES_CircuitDrawPort(vv, ckt, port,
		    r*cosf(theta),
		    r*sinf(theta));
	}

	/* TODO blend */
	if (com->selected && com->highlighted) {
		TAILQ_FOREACH(vn, &com->block->nodes, vgbmbs) {
			if (!(vn->flags & VG_NODE_SELECTED)) {
				VG_Select(vg, vn);
				VG_ColorRGB(vg, 250, 250, 180);
				VG_End(vg);
			}
		}
	} else if (com->selected) {
		TAILQ_FOREACH(vn, &com->block->nodes, vgbmbs) {
			if (!(vn->flags & VG_NODE_SELECTED)) {
				VG_Select(vg, vn);
				VG_ColorRGB(vg, 240, 240, 50);
				VG_End(vg);
			}
		}
	} else if (com->highlighted) {
		TAILQ_FOREACH(vn, &com->block->nodes, vgbmbs) {
			if (!(vn->flags & VG_NODE_SELECTED)) {
				VG_Select(vg, vn);
				VG_ColorRGB(vg, 180, 180, 120);
				VG_End(vg);
			}
		}
	}

	if (com->block->theta != 0)
		VG_RotateBlock(vg, com->block, com->block->theta);
}

static void
Init(void *obj)
{
	ES_Component *com = obj;

	com->ckt = NULL;
	com->block = NULL;
	com->selected = 0;
	com->highlighted = 0;
	com->Tspec = 27+273.15;
	com->nports = 0;
	com->pairs = NULL;
	com->npairs = 0;
	com->specs = NULL;
	com->nspecs = 0;
	
	com->loadDC_G = NULL;
	com->loadDC_BCD = NULL;
	com->loadDC_RHS = NULL;
	com->loadAC = NULL;
	com->loadSP = NULL;
	com->intStep = NULL;
	com->intUpdate = NULL;

	AG_SetEvent(com, "attached", OnAttach, NULL);
	AG_SetEvent(com, "detached", OnDetach, NULL);
	AG_SetEvent(com, "renamed", OnRename, NULL);
}

/* Initialize the ports of a component instance from a given model. */
void
ES_ComponentSetPorts(void *p, const ES_Port *ports)
{
	ES_Component *com = p;
	const ES_Port *pPort;
	ES_Pair *pair;
	int i, j, k;

	/* Instantiate the port array. */
	ES_ComponentFreePorts(com);
	com->nports = 0;
	com->ports[0].x = ports[0].x;
	com->ports[0].y = ports[0].y;
	for (i = 1, pPort = &ports[1];
	     i < COMPONENT_MAX_PORTS && pPort->n >= 0;
	     i++, pPort++) {
		ES_Port *port = &com->ports[i];

		Strlcpy(port->name, pPort->name, sizeof(port->name));
		port->n = i;
		port->x = pPort->x;
		port->y = pPort->y;
		port->com = com;
		port->node = -1;
		port->branch = NULL;
		port->flags = 0;
		com->nports++;
		Debug(com, "Added port #%d (%s)\n", i, port->name);
	}

	/* Find the port pairs. */
	com->pairs = Malloc(sizeof(ES_Pair));
	com->npairs = 0;
	for (i = 1; i <= com->nports; i++) {
		for (j = 1; j <= com->nports; j++) {
			if (i == j)
				continue;
			
			/* Skip the redundant pairs. */
			for (k = 0; k < com->npairs; k++) {
				ES_Pair *pair = &com->pairs[k];

				if (pair->p2 == &com->ports[i] &&
				    pair->p1 == &com->ports[j])
					break;
			}
			if (k < com->npairs)
				continue;

			com->pairs = Realloc(com->pairs,
			    (com->npairs+1)*sizeof(ES_Pair));
			pair = &com->pairs[com->npairs++];
			pair->com = com;
			pair->p1 = &com->ports[i];
			pair->p2 = &com->ports[j];
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
	int i;

	for (i = 1; i <= com->nports; i++) {
		ES_Port *port = &com->ports[i];

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

	DEV_BrowserOpenData(com);
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

	DEV_BrowserSaveTo(com, _(COMOPS(com)->name));
}

static void
LoadComponentFrom(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);

	DEV_BrowserLoadFrom(com, _(COMOPS(com)->name));
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
		extern VG_ToolOps esSelcomOps;
		extern VG_ToolOps esWireTool;

		AG_MenuAction(pm->item, _("Select"), NULL,
		    SelectTool, "%p,%p,%p", vgv, com->ckt, &esSelcomOps);
		AG_MenuAction(pm->item, _("Wire"), NULL,
		    SelectTool, "%p,%p,%p", vgv, com->ckt, &esWireTool);
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

/* Insert a new, floating component into a circuit. */
void
ES_ComponentInsert(AG_Event *event)
{
	extern ES_Component *esInscomCur;
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
	snprintf(name, sizeof(name), "%s%d", cls->pfx, n++);
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
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
		esInscomCur = com;
		esInscomCur->selected = 1;
	}
//	AG_WidgetFocus(vgv);
//	AG_WindowFocus(AG_WidgetParentWindow(vgv));
	
//	if (AG_ObjectSave(com) == -1)
//		AG_TextMsg(AG_MSG_ERROR, "%s", AG_GetError());
	
	ES_UnlockCircuit(ckt);
}

/* Return the port nearest the given VG coordinates. */
/* XXX */
ES_Port *
ES_PortNearestPoint(ES_Circuit *ckt, float x, float y, void *ignore)
{
	ES_Component *ocom;
	ES_Wire *wire;
	int i;
	
	CIRCUIT_FOREACH_COMPONENT(ocom, ckt) {
		if ((ocom == ignore) || (ocom->flags & COMPONENT_FLOATING)) {
			continue;
		}
		for (i = 1; i <= ocom->nports; i++) {
			ES_Port *oport = &ocom->ports[i];

			/* XXX arbitrary fuzz */
			if (fabs(x-ocom->block->pos.x - oport->x) < 0.25 &&
			    fabs(y-ocom->block->pos.y - oport->y) < 0.25)
				return (oport);
		}
	}
	TAILQ_FOREACH(wire, &ckt->wires, wires) {
		if (wire == ignore) {
			continue;
		}
		for (i = 0; i < 2; i++) {
			ES_Port *oport = &wire->ports[i];

			/* XXX arbitrary fuzz */
			if (fabs(x - oport->x) < 0.25 &&
			    fabs(y - oport->y) < 0.25) {
				return (oport);
			}
		}
	}
	return (NULL);
}

void
ES_UnselectAllPorts(ES_Circuit *ckt)
{
	ES_Component *com;
	ES_Wire *wire;
	int i;

	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		for (i = 1; i <= com->nports; i++)
			com->ports[i].flags &= ~(ES_PORT_SELECTED);
	}
	TAILQ_FOREACH(wire, &ckt->wires, wires) {
		for (i = 0; i < 2; i++)
			wire->ports[i].flags &= ~(ES_PORT_SELECTED);
	}
}

/* Evaluate whether the given port is connected to the reference point. */
int
ES_PortIsGrounded(ES_Port *port)
{
	ES_Circuit *ckt = port->com->ckt;

	return (port->node == 0);
}

/* Connect a floating component to the circuit. */
void
ES_ComponentConnect(ES_Circuit *ckt, ES_Component *com, VG_Vtx *vtx)
{
	ES_Port *oport;
	ES_Branch *br;
	int i;

	ES_LockCircuit(ckt);
	Debug(ckt, "Connecting component: %s\n", OBJECT(com)->name);
	Debug(com, "Connecting to circuit: %s\n", OBJECT(ckt)->name);
	for (i = 1; i <= com->nports; i++) {
		ES_Port *port = &com->ports[i];

		oport = ES_PortNearestPoint(ckt,
		    vtx->x + port->x,
		    vtx->y + port->y,
		    com);
		if (COMOPS(com)->connect == NULL ||
		    COMOPS(com)->connect(com, port, oport) == -1) {
			if (oport != NULL) {
#ifdef DEBUG
				if (port->node > 0) { Fatal("bad node"); }
#endif
				port->node = oport->node;
				port->branch = ES_CircuitAddBranch(ckt,
				    oport->node, port);
			} else {
				port->node = ES_CircuitAddNode(ckt, 0);
				port->branch = ES_CircuitAddBranch(ckt,
				    port->node, port);
			}
		}
		port->flags &= ~(ES_PORT_SELECTED);
	}
	com->flags &= ~(COMPONENT_FLOATING);

	AG_PostEvent(ckt, com, "circuit-connected", NULL);
	ES_CircuitModified(ckt);
	ES_UnlockCircuit(ckt);
}

/*
 * Highlight the connection points of the given component with respect
 * to other components in the circuit and return the number of matches.
 */
int
ES_ComponentHighlightPorts(ES_Circuit *ckt, ES_Component *com)
{
	ES_Port *oport;
	int i, nconn = 0;

	for (i = 1; i <= com->nports; i++) {
		ES_Port *nport = &com->ports[i];

		oport = ES_PortNearestPoint(ckt,
		    com->block->pos.x + nport->x,
		    com->block->pos.y + nport->y,
		    com);
		if (oport != NULL) {
			nport->flags |= ES_PORT_SELECTED;
			nconn++;
		} else {
			nport->flags &= ~(ES_PORT_SELECTED);
		}
	}
	return (nconn);
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
