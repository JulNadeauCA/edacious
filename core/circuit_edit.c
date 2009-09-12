/*
 * Copyright (c) 2008-2009 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Edition interface for circuits.
 */

#include "core.h"

/* Open a circuit sub-object for edition */
AG_Window *
ES_CircuitOpenObject(void *p)
{
	AG_Object *obj = p;
	AG_Object *objParent = AG_ObjectParent(obj);
	AG_Window *win;

	if ((win = AGOBJECT_CLASS(obj)->edit(obj)) == NULL) {
		AG_TextMsgFromError();
		return (NULL);
	}
	AG_WindowSetCaption(win, "<%s>: %s",
	    (objParent->archivePath != NULL) ?
	    AG_ShortFilename(objParent->archivePath) : objParent->name,
	    obj->name);

	AG_SetPointer(win, "object", objParent);
	AG_SetPointer(win, "circuit-object", obj);
	
	AG_WindowShow(win);
	return (win);
}

/* Close a circuit sub-object from edition */
void
ES_CircuitCloseObject(void *obj)
{
	AG_Window *win;
	AG_Variable *V;

	VIEW_FOREACH_WINDOW(win, agView) {
		if ((V = AG_GetVariableLocked(win,"circuit-object")) != NULL) {
			if (V->data.p == obj) {
				AG_ObjectDetach(win);
			}
			AG_UnlockVariable(V);
		}
	}
}

/* Update contents of "Show Topology / Loops" */
static void
PollCircuitLoops(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	int i;

	AG_TlistClear(tl);
	for (i = 0; i < ckt->l; i++) {
		ES_Loop *loop = ckt->loops[i];
		ES_Component *vSrc = loop->origin;
		AG_TlistItem *it;

		it = AG_TlistAdd(tl, NULL, "%s:L%u", OBJECT(vSrc)->name,
		    loop->name);
		it->p1 = loop;
	}
	AG_TlistRestore(tl);
}

/* Update contents of "Show Topology / Nodes" */
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
		AG_TlistItem *it;

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

/* Update contents of "Show Topology / Sources" */
static void
PollCircuitSources(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	AG_TlistItem *it;
	int i;

	AG_TlistClear(tl);
	for (i = 0; i < ckt->m; i++) {
		it = AG_TlistAddS(tl, NULL, OBJECT(ckt->vSrcs[i])->name);
		it->p1 = ckt->vSrcs[i];
	}
	AG_TlistRestore(tl);
}

/* Display circuit topology information */
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

/* Show "File / Properties..." dialog */
static void
ShowProperties(AG_Event *event)
{
	char path[AG_OBJECT_PATH_MAX];
	AG_Window *pwin = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	AG_Window *win;
	AG_Textbox *tb;
	
	AG_ObjectCopyName(ckt, path, sizeof(path));
	if ((win = AG_WindowNewNamed(0, "settings-%s", path)) == NULL) {
		return;
	}
	AG_WindowSetCaption(win, _("Circuit properties: %s"),
	    OBJECT(ckt)->name);
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 40, 30);
	
	tb = AG_TextboxNewS(win, AG_TEXTBOX_HFILL, _("Author: "));
	AG_TextboxBindUTF8(tb, ckt->authors, sizeof(ckt->authors));

	tb = AG_TextboxNewS(win, AG_TEXTBOX_HFILL, _("Keywords: "));
	AG_TextboxBindUTF8(tb, ckt->keywords, sizeof(ckt->keywords));

	AG_LabelNew(win, 0, _("Description: "));
	tb = AG_TextboxNewS(win, AG_TEXTBOX_EXPAND|AG_TEXTBOX_MULTILINE|
	                         AG_TEXTBOX_CATCH_TAB, NULL);
	AG_TextboxBindUTF8(tb, ckt->descr, sizeof(ckt->descr));
	AG_WidgetFocus(tb);

	AG_ButtonNewFn(win, AG_BUTTON_HFILL, _("Close"), AGWINCLOSE(win));

	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_BR, 30, 50);
	AG_WindowAttach(pwin, win);
	AG_WindowShow(win);
}

/* Process click on schematic area */
static void
MouseButtonDown(AG_Event *event)
{
	VG_View *vv =  AG_SELF();
	int button = AG_INT(1);
	float x = AG_FLOAT(2);
	float y = AG_FLOAT(3);
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

/* Simulation mode selected in "Simulation" menu */
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

/* Update contents of tree in "Objects" pane */
static void
FindObjects(AG_Tlist *tl, AG_Object *pob, int depth, void *ckt)
{
	AG_Object *cob;
	AG_TlistItem *it;

	if (AG_OfClass(pob, "ES_Circuit:ES_Component:*")) {
		if (ESCOMPONENT(pob)->flags & ES_COMPONENT_SUPPRESSED) {
			it = AG_TlistAdd(tl, esIconComponent.s,
			    _("%s (suppressed)"),
			    pob->name);
		} else {
			it = AG_TlistAddS(tl, esIconComponent.s, pob->name);
		}
		it->selected = (ESCOMPONENT(pob)->flags &
		                ES_COMPONENT_SELECTED);
	} else {
		it = AG_TlistAddS(tl, NULL, pob->name);
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

/* Update contents of tree in "Objects" pane */
static void
PollObjects(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	AG_Object *ckt = AG_PTR(1);

	AG_TlistClear(tl);
	AG_LockVFS(ckt);
	FindObjects(tl, ckt, 0, ckt);
	AG_UnlockVFS(ckt);
	AG_TlistRestore(tl);
}

/* Item selected in "Objects" pane */
static void
SelectedObject(AG_Event *event)
{
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

/* Display log console */
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

/* Create alternate schematic view */
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

/* Create new Scope object */
static void
NewScope(AG_Event *event)
{
	char name[AG_OBJECT_NAME_MAX];
	ES_Circuit *ckt = AG_PTR(1);
	AG_Window *winParent = AG_PTR(2);
	ES_Scope *scope;
	AG_Window *win;

	AG_ObjectGenName(ckt, &esScopeClass, name, sizeof(name));
	scope = ES_ScopeNew(ckt, name);

	if ((win = ES_CircuitOpenObject(scope)) != NULL)
		AG_WindowAttach(winParent, win);
}

/* Update list of PCB layouts */
static void
PollLayouts(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	AG_TlistItem *ti;
	ES_Layout *lo;

	AG_TlistBegin(tl);
	TAILQ_FOREACH(lo, &ckt->layouts, layouts) {
		ti = AG_TlistAddS(tl, NULL, OBJECT(lo)->name);
		ti->p1 = lo;
	}
	AG_TlistEnd(tl);
}

/* Open a PCB layout for edition */
static void
OpenLayout(AG_Event *event)
{
	AG_Window *winParent = AG_PTR(1);
	AG_TlistItem *ti = AG_PTR(2);
	ES_Layout *lo = ti->p1;
	AG_Window *win;

	if ((win = ES_CircuitOpenObject(lo)) != NULL)
		AG_WindowAttach(winParent, win);
}

/* Create a new PCB layout and open it for edition */
static void
NewLayout(AG_Event *event)
{
	char name[AG_OBJECT_NAME_MAX];
	ES_Circuit *ckt = AG_PTR(1);
	AG_Window *winParent = AG_PTR(2);
	AG_Tlist *tlLayouts = AG_PTR(3);
	ES_Layout *lo;
	AG_Window *win;

	if ((lo = ES_LayoutNew(ckt)) == NULL) {
		AG_TextMsgFromError();
		return;
	}
	AG_ObjectGenName(ckt, &esLayoutClass, name, sizeof(name));
	AG_ObjectSetNameS(lo, name);

	TAILQ_INSERT_TAIL(&ckt->layouts, lo, layouts);
	AG_TlistRefresh(tlLayouts);

	if ((win = ES_CircuitOpenObject(lo)) != NULL)
		AG_WindowAttach(winParent, win);
}

/* Delete a PCB layout */
static void
DeleteLayout(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	AG_Tlist *tlLayouts = AG_PTR(2);
	AG_TlistItem *ti = AG_TlistSelectedItem(tlLayouts);
	ES_Layout *lo;

	if (ti == NULL) { return; }
	lo = ti->p1;

	ES_CircuitCloseObject(lo);

	TAILQ_REMOVE(&ckt->layouts, lo, layouts);
	AG_ObjectDetach(lo);
	AG_ObjectDestroy(lo);

	AG_TlistRefresh(tlLayouts);
}

void *
ES_CircuitEdit(void *p)
{
	ES_Circuit *ckt = p;
	AG_Window *win;
	AG_Toolbar *tbTop, *tbRight;
	AG_Pane *hPane, *vPane;
	VG_View *vv;
	AG_Menu *menu;
	AG_MenuItem *mi, *mi2;

	win = AG_WindowNew(0);

	tbTop = AG_ToolbarNew(NULL, AG_TOOLBAR_HORIZ, 1, 0);
	tbRight = AG_ToolbarNew(NULL, AG_TOOLBAR_VERT, 1, 0);
	AG_ExpandHoriz(tbTop);
	AG_ExpandVert(tbRight);
	
	vv = VG_ViewNew(NULL, ckt->vg, VG_VIEW_EXPAND|VG_VIEW_GRID);
	VG_ViewSetSnapMode(vv, VG_GRID);
	VG_ViewSetScale(vv, 2);

	menu = AG_MenuNew(win, AG_MENU_HFILL);
	mi = AG_MenuAddItem(menu, _("File"));
	{
		AG_MenuAction(mi, _("Properties..."), agIconGear.s,
		    ShowProperties, "%p,%p,%p", win, ckt, vv);
	}
	mi = AG_MenuAddItem(menu, _("Edit"));
	{
		mi2 = AG_MenuNode(mi, _("Snapping mode"), NULL);
		VG_SnapMenu(mi2, vv);
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

		mi2 = AG_MenuNode(mi, _("Display"), esIconCircuit.s);
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
#ifdef ES_DEBUG
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
		AG_NotebookTab *nt;
		AG_Tlist *tl;
		ES_ComponentLibraryEditor *led;
		AG_Box *vBox, *hBox;

		vPane = AG_PaneNewVert(hPane->div[0], AG_PANE_EXPAND);
		nb = AG_NotebookNew(vPane->div[0], AG_NOTEBOOK_EXPAND);
		nt = AG_NotebookAddTab(nb, _("Library"), AG_BOX_VERT);
		{
			led = ES_ComponentLibraryEditorNew(nt, vv, ckt, 0);
			AG_WidgetSetFocusable(led, 0);
		}
		nt = AG_NotebookAddTab(nb, _("Objects"), AG_BOX_VERT);
		{
			tl = AG_TlistNewPolled(nt,
			    AG_TLIST_TREE|AG_TLIST_EXPAND|AG_TLIST_NOSELSTATE,
			    PollObjects, "%p", ckt);
			AG_TlistSetRefresh(tl, 500);
			AG_SetEvent(tl, "tlist-changed",
			    SelectedObject, "%p", vv);
			AG_WidgetSetFocusable(tl, 0);
		}
		nt = AG_NotebookAddTab(nb, _("PCBs"), AG_BOX_VERT);
		{
			tl = AG_TlistNewPolled(nt, AG_TLIST_TREE|AG_TLIST_EXPAND,
			    PollLayouts, "%p", ckt);
			AG_TlistSetRefresh(tl, -1);
			AG_TlistSetDblClickFn(tl,
			    OpenLayout, "%p", win);
			AG_WidgetSetFocusable(tl, 0);

			hBox = AG_BoxNewHorizNS(nt, AG_BOX_HFILL|AG_BOX_HOMOGENOUS);
			{
				AG_ButtonNewFn(hBox, 0, _("New"),
				    NewLayout, "%p,%p,%p", ckt, win, tl);
				AG_ButtonNewFn(hBox, 0, _("Delete"),
				    DeleteLayout, "%p,%p", ckt, tl);
			}
		}

		VG_AddEditArea(vv, vPane->div[1]);

		vBox = AG_BoxNewVertNS(hPane->div[1], AG_BOX_EXPAND);
		{
			AG_ObjectAttach(vBox, tbTop);
			hBox = AG_BoxNewHorizNS(vBox, AG_BOX_EXPAND);
			{
				AG_ObjectAttach(hBox, vv);
				AG_ObjectAttach(hBox, tbRight);
			}
		}

		AG_WidgetFocus(vv);
	}

	mi = AG_MenuAddItem(menu, _("Tools"));
	{
		AG_MenuItem *mAction;
		VG_ToolOps **pOps, *ops;
		VG_Tool *tool;

		AG_MenuToolbar(mi, tbRight);
	
		/* Register Circuit-specific tools */
		for (pOps = &esCircuitTools[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			tool = VG_ViewRegTool(vv, ops, ckt);
			mAction = AG_MenuAction(mi, ops->name,
			    ops->icon ? ops->icon->s : NULL,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, ckt);
			AG_MenuSetIntBoolMp(mAction, &tool->selected, 0,
			    &OBJECT(vv)->lock);
			if (ops == &esSchemSelectTool) {
				VG_ViewSetDefaultTool(vv, tool);
				VG_ViewSelectTool(vv, tool, ckt);
			}
		}
		
		AG_MenuSeparator(mi);
		
		/* Register generic VG drawing tools */
		for (pOps = &esVgTools[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			if (ops == &vgSelectTool) {
				continue;	/* Use alternate "select" */
			}
			tool = VG_ViewRegTool(vv, ops, ckt);
			mAction = AG_MenuAction(mi, ops->name,
			    ops->icon ? ops->icon->s : NULL,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, ckt);
			AG_MenuSetIntBoolMp(mAction, &tool->selected, 0,
			    &OBJECT(vv)->lock);
		}

		/* Register (but hide) the special "insert component" tool. */
		VG_ViewRegTool(vv, &esComponentInsertTool, ckt);

		VG_ViewButtondownFn(vv, MouseButtonDown, NULL);
		
		AG_MenuToolbar(mi, NULL);
	}
	
	mi = AG_MenuAddItem(menu, _("Visualization"));
	{
		AG_MenuToolbar(mi, tbTop);
		AG_MenuAction(mi, _("New scope..."), esIconScope.s,
		    NewScope, "%p,%p", ckt, win);
		AG_MenuToolbar(mi, NULL);
	}
	
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 85, 85);
	return (win);
}
