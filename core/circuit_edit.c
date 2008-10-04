/*
 * Copyright (c) 2008 Hypertriton, Inc. <http://hypertriton.com/>
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
	
	tb = AG_TextboxNew(win, 0, _("Author: "));
	AG_TextboxBindUTF8(tb, ckt->authors, sizeof(ckt->authors));

	tb = AG_TextboxNew(win, 0, _("Keywords: "));
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
	char name[AG_FILENAME_MAX];
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
	scope = ES_ScopeNew(&esVfsRoot, name, ckt);
	ES_OpenObject(scope);
}

static void
InsertComponent(AG_Event *event)
{
	VG_View *vv = AG_PTR(1);
	AG_Tlist *tl = AG_PTR(2);
	ES_Circuit *ckt = AG_PTR(3);
	AG_TlistItem *ti = AG_PTR(4);
	ES_ComponentClass *comCls = ti->p1;
	VG_Tool *insTool;

	if ((insTool = VG_ViewFindTool(vv, "Insert component")) != NULL) {
		VG_ViewSelectTool(vv, insTool, ckt);
		ES_InsertComponent(ckt, insTool, comCls);
	}
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
	VG_ViewSetScale(vv, 0);

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
		AG_Box *vBox, *hBox;

		vPane = AG_PaneNewVert(hPane->div[0], AG_PANE_EXPAND);
		nb = AG_NotebookNew(vPane->div[0], AG_NOTEBOOK_EXPAND);
		ntab = AG_NotebookAddTab(nb, _("Models"), AG_BOX_VERT);
		{
			ES_ComponentClass *cls;
			int i;

			tl = AG_TlistNewPolled(ntab,
			    AG_TLIST_EXPAND|AG_TLIST_TREE,
			    ES_ComponentListModels, NULL);
			AG_TlistSizeHint(tl, "XXXXXXXXXXXXXXXXXXX", 10);

			AG_WidgetSetFocusable(tl, 0);
			AG_SetEvent(tl, "tlist-dblclick",
			    InsertComponent, "%p,%p,%p", vv, tl, ckt);
		}
		ntab = AG_NotebookAddTab(nb, _("Objects"), AG_BOX_VERT);
		{
			tl = AG_TlistNewPolled(ntab,
			    AG_TLIST_POLL|AG_TLIST_TREE|AG_TLIST_EXPAND|
			    AG_TLIST_NOSELSTATE,
			    PollObjects, "%p", ckt);
			AG_SetEvent(tl, "tlist-changed",
			    SelectedObject, "%p", vv);
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
	
	mi = AG_MenuAddItem(menu, _("Visualization"));
	{
		AG_MenuToolbar(mi, tbTop);
		AG_MenuAction(mi, _("New scope..."), esIconScope.s,
		    NewScope, "%p", ckt);
		AG_MenuToolbar(mi, NULL);
	}
	
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 85, 85);
	return (win);
}
