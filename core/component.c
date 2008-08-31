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

/* Free all Port information in the given Component. */
void
ES_FreePorts(ES_Component *com)
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
	VG_Node *vn, *vnNext;

	for (vn = TAILQ_FIRST(&com->schemEnts);
	     vn != TAILQ_END(&com->schemEnts);
	     vn = vnNext) {
		vnNext = TAILQ_NEXT(vn, user);
		VG_NodeDetach(vn);
		VG_NodeDestroy(vn);
	}
	TAILQ_INIT(&com->schemEnts);

	ES_FreePorts(com);
}

static void
Destroy(void *p)
{
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

/* Load a component schematic block from a file. */
ES_SchemBlock *
ES_LoadSchemFromFile(void *pCom, const char *path)
{
	ES_Component *com = pCom;
	ES_SchemBlock *sb;

	sb = ES_SchemBlockNew(com->ckt->vg->root, OBJECT(com)->name);
	if (ES_SchemBlockLoad(sb, path) == -1) {
		VG_NodeDetach(sb);
		VG_NodeDestroy(sb);
		return (NULL);
	}
	return (sb);
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
	
	if (!AG_OfClass(ckt, "ES_Circuit:*"))
		return;

	ES_LockCircuit(ckt);
	Debug(ckt, "Attach %s\n", OBJECT(com)->name);
	com->ckt = ckt;

	if (COMCLASS(com)->schemFile != NULL) {
		ES_SchemBlock *sb;
		char path[AG_FILENAME_MAX];

#ifdef _WIN32
		Strlcpy(path, "Schematics\\", sizeof(path));
#else
		Strlcpy(path, SHAREDIR, sizeof(path));
		Strlcat(path, "/Schematics/", sizeof(path));
#endif
		Strlcat(path, COMCLASS(com)->schemFile, sizeof(path));
		if ((sb = ES_LoadSchemFromFile(com, path)) != NULL) {
			ES_AttachSchemEntity(com, VGNODE(sb));
		} else {
			AG_TextMsgFromError();
		}
	}
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
	Debug(ckt, "Detach %s\n", OBJECT(com)->name);
	AG_PostEvent(ckt, com, "circuit-disconnected", NULL);

	while ((vn = TAILQ_FIRST(&com->schemEnts)) != NULL) {
		Debug(com, "Removing schematic entity: %s%u\n",
		    vn->ops->name, vn->handle);
		ES_DetachSchemEntity(com, vn);
		if (VG_Delete(vn) == -1)
			AG_FatalError("Deleting node: %s", AG_GetError());
	}

del_branches:
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
	/* XXX hack */
	for (i = 1; i < ckt->n; i++) {
		ES_Node *node = ckt->nodes[i];
		ES_Branch *br;

		if (node->nBranches == 0) {
			Debug(com, "n%u has no more branches; removing\n", i);
			ES_DelNode(ckt, i);
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

	/* Instantiate the port array. */
	ES_FreePorts(com);
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
		}
	}
}

static int
Load(void *p, AG_DataSource *ds, const AG_Version *ver)
{
	ES_Component *com = p;

	com->flags = (Uint)AG_ReadUint32(ds);
	com->Tspec = M_ReadReal(ds);
	return (0);
}

static int
Save(void *p, AG_DataSource *ds)
{
	ES_Component *com = p;

	AG_WriteUint32(ds, com->flags & ES_COMPONENT_SAVED_FLAGS);
	M_WriteReal(ds, com->Tspec);
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
#ifdef DEBUG
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

static void
Delete(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_View *vv = AG_PTR(2);
	ES_Circuit *ckt = com->ckt;
	
	ES_ClearEditAreas(vv);
	ES_LockCircuit(ckt);
	
	if (!AG_ObjectInUse(com)) {
		VG_Status(vv, _("Removed component %s."), OBJECT(com)->name);
		AG_ObjectDetach(com);
		AG_ObjectDestroy(com);
		ES_CircuitModified(ckt);
	} else {
		VG_Status(vv, _("Cannot delete %s: Component is in use!"),
		    OBJECT(com)->name);
	}
	ES_UnlockCircuit(ckt);
}

static void
Rotate(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	float theta = AG_FLOAT(2);
	VG_Node *vn;
	
	TAILQ_FOREACH(vn, &com->schemEnts, user)
		VG_Rotate(vn, theta);
}

static void
FlipHoriz(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_Node *vn;
	
	TAILQ_FOREACH(vn, &com->schemEnts, user)
		VG_FlipHoriz(vn);
}

static void
FlipVert(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_Node *vn;
	
	TAILQ_FOREACH(vn, &com->schemEnts, user)
		VG_FlipVert(vn);
}

static void
SuppressComponent(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_View *vv = AG_PTR(2);
	ES_Circuit *ckt = com->ckt;
	
	ES_LockCircuit(ckt);
	VG_Status(vv, _("Suppressed component %s."), OBJECT(com)->name);
	com->flags |= ES_COMPONENT_SUPPRESSED;
	ES_UnlockCircuit(ckt);
}

static void
UnsuppressComponent(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_View *vv = AG_PTR(2);
	ES_Circuit *ckt = com->ckt;
	
	ES_LockCircuit(ckt);
	VG_Status(vv, _("Unsuppressed component %s."), OBJECT(com)->name);
	com->flags &= ~(ES_COMPONENT_SUPPRESSED);
	ES_UnlockCircuit(ckt);
}

static void
DeleteSelections(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	VG_View *vv = AG_PTR(2);
	ES_Component *com;
	int changed = 0;

	ES_ClearEditAreas(vv);
	ES_LockCircuit(ckt);
rescan:
	CIRCUIT_FOREACH_COMPONENT_SELECTED(com, ckt) {
		if (!AG_ObjectInUse(com)) {
			AG_ObjectDetach(com);
			AG_ObjectDestroy(com);
			changed++;
		} else {
			VG_Status(vv,
			    _("Cannot delete %s: Component is in use!"),
			    OBJECT(com)->name);
		}
		goto rescan;
	}
	if (changed) {
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
SelectTool(AG_Event *event)
{
	VG_View *vgv = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	VG_ToolOps *ops = AG_PTR(3);
	VG_Tool *t;

	if ((t = VG_ViewFindToolByOps(vgv, ops)) != NULL) {
		VG_ViewSelectTool(vgv, t, ckt);
	} else {
		AG_TextMsg(AG_MSG_ERROR, _("No such tool: %s"), ops->name);
	}
}

static void
PollPorts(AG_Event *event)
{
	char text[AG_TLIST_LABEL_MAX];
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	ES_Component *com;
	ES_Port *port;
	int i;
	
	AG_TlistClear(tl);
	
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->flags & ES_COMPONENT_SELECTED)
			break;
	}
	if (com == NULL)
		goto out;

	AG_ObjectLock(com);
	COMPONENT_FOREACH_PORT(port, i, com) {
		Snprintf(text, sizeof(text), "%d (%s) -> n%d [%.03fV]",
		    port->n, port->name, port->node,
		    ES_NodeVoltage(ckt, port->node));
		AG_TlistAddPtr(tl, NULL, text, port);
	}
	AG_ObjectUnlock(com);
out:
	AG_TlistRestore(tl);
}

static void
PollPairs(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	ES_Component *com;
	int i, j;
	
	AG_TlistClear(tl);
	
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->flags & ES_COMPONENT_SELECTED)
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
PortInfo(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	AG_Window *win;
	AG_Tlist *tl;

	if ((win = AG_WindowNewNamed(0, "%s-port-info", OBJECT(com)->name))
	    == NULL) {
		return;
	}
	tl = AG_TlistNew(win, AG_TLIST_POLL|AG_TLIST_EXPAND);
	AG_TlistSizeHint(tl, "1 (PORT) -> N123 [0.000V]", 4);
	AG_SetEvent(tl, "tlist-poll", PollPorts, "%p", com->ckt);
	AG_ButtonNewFn(win, AG_BUTTON_HFILL, _("Close"), AGWINCLOSE(win));
	AG_WindowShow(win);
}

static void
PairInfo(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	AG_Window *win;
	AG_Tlist *tl;

	if ((win = AG_WindowNewNamed(0, "%s-pair-info", OBJECT(com)->name))
	    == NULL) {
		return;
	}
	tl = AG_TlistNew(win, AG_TLIST_POLL|AG_TLIST_EXPAND);
	AG_SetEvent(tl, "tlist-poll", PollPairs, "%p", com->ckt);
	AG_ButtonNewFn(win, AG_BUTTON_HFILL, _("Close"), AGWINCLOSE(win));
	AG_WindowShow(win);
}

/* Generate a popup menu for the given Component. */
void
ES_ComponentMenu(ES_Component *com, VG_View *vgv)
{
	AG_PopupMenu *pm;
	AG_MenuItem *mi, *mi2;
	Uint nsel = 0;
	ES_Component *com2;
	int common_class = 1;
	ES_ComponentClass *comCls = NULL;

	CIRCUIT_FOREACH_COMPONENT_SELECTED(com2, com->ckt) {
		if (comCls != NULL && comCls != COMCLASS(com2)) {
			common_class = 0;
		}
		comCls = COMCLASS(com2);
		nsel++;
	}

	pm = AG_PopupNew(vgv);
	mi = pm->item;
	{
		extern VG_ToolOps esCircuitSelectTool;
		extern VG_ToolOps esWireTool;

		AG_MenuState(mi, (vgv->curtool == NULL) ||
		                 (vgv->curtool->ops != &esSchemSelectTool));
		AG_MenuAction(mi, _("Select tool"), esIconSelectArrow.s,
		    SelectTool, "%p,%p,%p", vgv, com->ckt,
		    &esSchemSelectTool);

		AG_MenuState(mi, (vgv->curtool == NULL) ||
		                 (vgv->curtool->ops != &esWireTool));
		AG_MenuAction(mi, _("Wire tool"), esIconInsertWire.s,
		    SelectTool, "%p,%p,%p", vgv, com->ckt,
		    &esWireTool);

		AG_MenuState(mi, 1);
		AG_MenuSeparator(mi);
	}

	AG_MenuSection(mi, _("[Component: %s]"), OBJECT(com)->name);

	mi2 = AG_MenuNode(mi, _("Transform"), esIconRotate.s);
	{
		AG_MenuAction(mi2, _("Rotate 90\xc2\xb0 CW"),
		    esIconRotate.s,
		    Rotate, "%p,%f", com, M_PI/2.0f);
		AG_MenuAction(mi2, _("Rotate 90\xc2\xb0 CCW"),
		    esIconRotate.s,
		    Rotate, "%p,%f", com, -M_PI/2.0f);
		AG_MenuSeparator(mi2);
		AG_MenuAction(mi2, _("Rotate 45\xc2\xb0 CW"),
		    esIconRotate.s,
		    Rotate, "%p,%f", com, M_PI/4.0f);
		AG_MenuAction(mi2, _("Rotate 45\xc2\xb0 CCW"),
		    esIconRotate.s,
		    Rotate, "%p,%f", com, -M_PI/4.0f);
		AG_MenuSeparator(mi2);
		AG_MenuAction(mi2, _("Flip horizontally"), esIconFlipHoriz.s,
		    FlipHoriz, "%p", com);
		AG_MenuAction(mi2, _("Flip vertically"), esIconFlipVert.s,
		    FlipVert, "%p", com);
	}
	
	AG_MenuAction(mi, _("Delete"), agIconTrash.s,
	    Delete, "%p,%p", com, vgv);
	if (com->flags & ES_COMPONENT_SUPPRESSED) {
		AG_MenuState(mi, 1);
		AG_MenuAction(mi, _("Unsuppress"), esIconStartSim.s,
		    UnsuppressComponent, "%p,%p", com, vgv);
		AG_MenuState(mi, 0);
	} else {
		AG_MenuAction(mi, _("Suppress"), esIconStopSim.s,
		    SuppressComponent, "%p,%p", com, vgv);
	}

	AG_MenuSeparator(mi);
	AG_MenuAction(mi, _("Port information..."), esIconPortEditor.s,
	    PortInfo, "%p,%p", com, vgv);

	if (COMCLASS(com)->instance_menu != NULL) {
		AG_MenuSeparator(mi);
		COMCLASS(com)->instance_menu(com, mi);
	}
	if (nsel > 1) {
		if (common_class && COMCLASS(com)->class_menu != NULL) {
			AG_MenuSeparator(mi);
			AG_MenuSection(mi, _("[Class: %s]"), comCls->name);
			COMCLASS(com)->class_menu(com->ckt, mi);
		}
		AG_MenuSeparator(mi);
		AG_MenuSection(mi, _("[All selections]"));
		AG_MenuAction(mi, _("Delete selected"), agIconTrash.s,
		    DeleteSelections, "%p, %p", com->ckt, vgv);
	}

	AG_PopupShow(pm);
}

static void
CreateView(AG_Event *event)
{
	AG_Window *pWin = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	AG_Window *win;
	VG_View *vv;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("View of %s"), OBJECT(ckt)->name);
	vv = VG_ViewNew(win, ckt->vg, VG_VIEW_EXPAND);
	AG_WidgetFocus(vv);
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_TR, 60, 50);
	AG_WindowAttach(pWin, win);
	AG_WindowShow(win);
}

static void
InsertComponent(AG_Event *event)
{
	VG_View *vv = AG_PTR(1);
	AG_Tlist *tl = AG_PTR(2);
	ES_Circuit *ckt = AG_PTR(3);
	AG_TlistItem *it;
	VG_Tool *insTool;

	if ((it = AG_TlistSelectedItem(tl)) == NULL) {
		AG_TextMsg(AG_MSG_ERROR, _("No component type is selected."));
		return;
	}
	if ((insTool = VG_ViewFindTool(vv, "Insert component")) != NULL) {
		VG_ViewSelectTool(vv, insTool, ckt);
		ES_InsertComponent(ckt, insTool, it->p1);
	}
}

static void
FindObjects(AG_Tlist *tl, AG_Object *pob, int depth, void *ckt)
{
	AG_Object *cob;
	AG_TlistItem *it;
	int selected = 0;

	if (AG_OfClass(pob, "ES_Circuit:ES_Component:*")) {
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

	if (AG_OfClass(obj, "ES_Circuit:ES_Component:*")) {
		if (state) {
			ES_SelectComponent(COMPONENT(obj), vv);
		} else {
			ES_UnselectComponent(COMPONENT(obj), vv);
		}
	}
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

static void *
Edit(void *obj)
{
	ES_Component *com = obj;
	ES_Circuit *ckt = obj;
	AG_Toolbar *tbTop, *tbRight;
	AG_Pane *hPane, *vPane;
	VG_View *vv;
	AG_Window *win;
	AG_Menu *menu;
	AG_MenuItem *mi, *mi2;
	AG_Notebook *nb;
	AG_NotebookTab *nt;
	AG_Widget *wEdit;
	AG_Box *box;

	win = AG_WindowNew(0);

	/*
	 * Equivalent circuit editor
	 */
	vv = VG_ViewNew(NULL, ckt->vg, VG_VIEW_EXPAND|VG_VIEW_GRID);
	VG_ViewSetSnapMode(vv, VG_GRID);
	VG_ViewSetScale(vv, 0);
	tbTop = AG_ToolbarNew(NULL, AG_TOOLBAR_HORIZ, 1, 0);
	tbRight = AG_ToolbarNew(NULL, AG_TOOLBAR_VERT, 1, 0);
	AG_ExpandHoriz(tbTop);
	AG_ExpandVert(tbRight);

	/*
	 * Menu
	 */
	menu = AG_MenuNew(win, AG_MENU_HFILL);
	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);
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

			AG_MenuSeparator(mi2);

			AG_MenuFlags(mi2, _("Grid"), vgIconSnapGrid.s,
			    &vv->flags, VG_VIEW_GRID, 0);
			AG_MenuFlags(mi2, _("Construction geometry"),
			    esIconConstructionGeometry.s,
			    &vv->flags, VG_VIEW_CONSTRUCTION, 0);
		}
	}
	
	nt = AG_NotebookAddTab(nb, _("Schematic"), AG_BOX_VERT);
	{
	}
	
	/*
	 * Equivalent circuit editor
	 */
	nt = AG_NotebookAddTab(nb, _("Equivalent Circuit"), AG_BOX_VERT);
	hPane = AG_PaneNewHoriz(nt, AG_PANE_EXPAND);
	{
		AG_Notebook *nb;
		AG_NotebookTab *ntab;
		AG_Tlist *tl;
		AG_Box *vBox, *hBox;

		vPane = AG_PaneNewVert(hPane->div[0], AG_PANE_EXPAND);
		nb = AG_NotebookNew(vPane->div[0], AG_NOTEBOOK_EXPAND);
		ntab = AG_NotebookAddTab(nb, _("Models"), AG_BOX_VERT);
		{
			ES_ComponentClass *cls;
			int i;

			tl = AG_TlistNew(ntab, AG_TLIST_EXPAND);
			AG_WidgetSetFocusable(tl, 0);
			AG_SetEvent(tl, "tlist-dblclick", InsertComponent,
			    "%p,%p,%p", vv, tl, ckt);
			    
			AG_FOREACH_CLASS(cls, i, es_component_class,
			    "ES_Circuit:ES_Component:*") {
				if (strcmp(AGCLASS(cls)->name,
				    "ES_Circuit:ES_Component") == 0) {
					continue;
				}
				AG_TlistAddPtr(tl,
				    cls->icon != NULL ? cls->icon->s : NULL,
				    cls->name, cls);
			}
			AG_TlistSizeHintLargest(tl, 10);
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

		vBox = AG_BoxNewVert(hPane->div[1], AG_BOX_EXPAND);
		AG_BoxSetSpacing(vBox, 0);
		AG_BoxSetPadding(vBox, 0);
		{
			AG_ObjectAttach(vBox, tbTop);
			hBox = AG_BoxNewHoriz(vBox, AG_BOX_EXPAND);
			AG_BoxSetSpacing(hBox, 0);
			AG_BoxSetPadding(hBox, 0);
			AG_ObjectAttach(hBox, vv);
			AG_ObjectAttach(hBox, tbRight);
			AG_WidgetFocus(vv);
		}
	}
	mi = AG_MenuAddItem(menu, _("Tools"));
	{
		AG_MenuItem *mAction;
		VG_ToolOps **pOps, *ops;
		VG_Tool *tool;

		AG_MenuToolbar(mi, tbRight);
		for (pOps = &esCircuitTools[0]; *pOps != NULL; pOps++) {
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
		for (pOps = &esSchemTools[0]; *pOps != NULL; pOps++) {
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
		AG_MenuToolbar(mi, NULL);
	}

	/*
	 * Component Parameters Editor
	 */
	nt = AG_NotebookAddTab(nb, _("Model Parameters"), AG_BOX_VERT);
	{
		AG_Textbox *tb;

		hPane = AG_PaneNewHoriz(nt, AG_PANE_EXPAND);
		if (OBJECT_CLASS(com)->edit != NULL) {
			wEdit = OBJECT_CLASS(com)->edit(com);
			AG_ObjectAttach(hPane->div[0], wEdit);
		}
		AG_LabelNewStatic(hPane->div[1], 0, _("Notes:"));
		tb = AG_TextboxNew(hPane->div[1],
		    AG_TEXTBOX_EXPAND|AG_TEXTBOX_MULTILINE, NULL);
	}

	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 85, 85);
	return (win);
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
		Edit
	},
	N_("Component"),
	"U",
	NULL,			/* schemFile */
	"Generic",
	&esIconComponent,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
