/*
 * Copyright (c) 2009 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Layout editor interface.
 */

#include "core.h"

/* Create an alternate view of the layout */
/* TODO: 3D projection */
static void
CreateView(AG_Event *event)
{
	AG_Window *pwin = AG_PTR(1);
	ES_Layout *lo = AG_PTR(2);
	VG_View *vv;
	AG_Window *win;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("View of %s"), OBJECT(lo)->name);

	vv = VG_ViewNew(win, lo->vg, VG_VIEW_EXPAND);
	AG_WidgetFocus(vv);
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_TR, 60, 50);
	AG_WindowAttach(pwin, win);
	AG_WindowShow(win);
}

/* Update the list of components and related device packages */
static void
PollDevicePkgs(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Layout *lo = AG_PTR(1);
	ES_Circuit *ckt = lo->ckt;
	ES_Component *com;
	ES_ComponentPkg *pkg;
	AG_TlistItem *ti;

	AG_TlistBegin(tl);
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->flags & ES_COMPONENT_SPECIAL) {
			continue;
		}
		ti = AG_TlistAdd(tl, esIconComponent.s, "%s", OBJECT(com)->name);
		ti->depth = 0;
		ti->cat = "component";
		ti->p1 = com;
		TAILQ_FOREACH(pkg, &com->pkgs, pkgs) {
			ti = AG_TlistAdd(tl, esIconComponent.s, "%s", pkg->name);
			ti->depth = 1;
			ti->cat = "package";
			ti->p1 = pkg;
		}
	}
	AG_TlistEnd(tl);
}

/* Insert a device package layout for a component. */
static void
InsertDevicePkg(AG_Event *event)
{
	VG_View *vv = AG_PTR(1);
	ES_Layout *lo = AG_PTR(2);
	AG_TlistItem *ti = AG_PTR(3);
	ES_Package *pkg = ti->p1;
	VG_Tool *insTool;

	if (strcmp(ti->cat, "package") != 0)
		return;

	if ((insTool = VG_ViewFindToolByOps(vv, &esPackageInsertTool)) == NULL) {
		AG_TextMsgFromError();
		return;
	}
	VG_ViewSelectTool(vv, insTool, lo);
	if (VG_ToolCommandExec(insTool, "Insert",
	    "%p,%p,%p", insTool, lo, pkg) == -1)
		AG_TextMsgFromError();
}

/* Update the list of layers */
static void
PollLayers(AG_Event *event)
{
//	AG_Tlist *tl = AG_SELF();
//	ES_Layout *lo = AG_PTR(1);
}

void *
ES_LayoutEdit(void *p)
{
	ES_Layout *lo = p;
	AG_Window *win;
	AG_Toolbar *tbRight;
	AG_Pane *hPane;
	VG_View *vv;
	AG_Menu *menu;
	AG_MenuItem *mi, *mi2;

	win = AG_WindowNew(0);

	tbRight = AG_ToolbarNew(NULL, AG_TOOLBAR_VERT, 1, 0);
	AG_ExpandVert(tbRight);
	
	vv = VG_ViewNew(NULL, lo->vg, VG_VIEW_EXPAND|VG_VIEW_GRID|
	                              VG_VIEW_CONSTRUCTION);
	VG_ViewSetSnapMode(vv, VG_GRID);
	VG_ViewSetScale(vv, 0);

	menu = AG_MenuNew(win, AG_MENU_HFILL);
#if 0
	mi = AG_MenuAddItem(menu, _("File"));
	{
		AG_MenuAction(mi, _("Properties..."), agIconGear.s,
		    ShowProperties, "%p,%p,%p", win, lo, vv);
	}
#endif
	mi = AG_MenuAddItem(menu, _("Edit"));
	{
		mi2 = AG_MenuNode(mi, _("Snapping mode"), NULL);
		VG_SnapMenu(mi2, vv);
	}
	mi = AG_MenuAddItem(menu, _("View"));
	{
		AG_MenuActionKb(mi, _("New view..."), esIconCircuit.s,
		    SDLK_v, KMOD_CTRL,
		    CreateView, "%p,%p", win, lo);

		AG_MenuSeparator(mi);

		mi2 = AG_MenuNode(mi, _("Display"), esIconCircuit.s);
		{
			AG_MenuFlags(mi2, _("Grid"), vgIconSnapGrid.s,
			    &vv->flags, VG_VIEW_GRID, 0);
			AG_MenuFlags(mi2, _("Construction geometry"),
			    esIconConstructionGeometry.s,
			    &vv->flags, VG_VIEW_CONSTRUCTION, 0);
		}
	}
	
	hPane = AG_PaneNewHoriz(win, AG_PANE_EXPAND);
	{
		AG_Notebook *nb;
		AG_NotebookTab *nt;
		AG_Box *vBox, *hBox;
		AG_Pane *vPane;
		AG_Tlist *tl;
		
		vPane = AG_PaneNewVert(hPane->div[0], AG_PANE_EXPAND);
		nb = AG_NotebookNew(vPane->div[0], AG_NOTEBOOK_EXPAND);
		
		if (lo->ckt != NULL) {
			nt = AG_NotebookAddTab(nb, _("Devices"), AG_BOX_VERT);
			tl = AG_TlistNewPolled(nt,
			    AG_TLIST_TREE|AG_TLIST_EXPAND|AG_TLIST_NOSELSTATE,
			    PollDevicePkgs, "%p", lo);
			AG_TlistSizeHint(tl, "<XXXXXXXXX>", 10);
			AG_TlistSetRefresh(tl, 1000);
			AG_WidgetSetFocusable(tl, 0);
			AG_TlistSetDblClickFn(tl,
			    InsertDevicePkg, "%p,%p", vv, lo);
		}
		nt = AG_NotebookAddTab(nb, _("Layers"), AG_BOX_VERT);
		{
			tl = AG_TlistNewPolled(nt,
			    AG_TLIST_TREE|AG_TLIST_EXPAND|AG_TLIST_NOSELSTATE,
			    PollLayers, "%p", lo);
			AG_TlistSetRefresh(tl, -1);
			AG_WidgetSetFocusable(tl, 0);
#if 0
			hBox = AG_BoxNewHorizNS(nt, AG_BOX_HFILL|AG_BOX_HOMOGENOUS);
			{
				AG_ButtonNewFn(hBox, 0, _("New"),
				    NewLayer, "%p,%p,%p", ckt, win, tl);
				AG_ButtonNewFn(hBox, 0, _("Delete"),
				    DeleteLayer, "%p,%p", ckt, tl);
			}
#endif
		}

		VG_AddEditArea(vv, vPane->div[1]);

		vBox = AG_BoxNewVertNS(hPane->div[1], AG_BOX_EXPAND);
		{
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
		
		/* Register Layout-specific tools */
		for (pOps = &esLayoutTools[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			tool = VG_ViewRegTool(vv, ops, NULL);
			mAction = AG_MenuAction(mi, ops->name,
			    ops->icon ? ops->icon->s : NULL,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, lo);
			AG_MenuSetIntBoolMp(mAction, &tool->selected, 0,
			    &OBJECT(vv)->lock);
		}
		
		AG_MenuSeparator(mi);
		
		/* Register generic VG drawing tools */
		for (pOps = &esVgTools[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			tool = VG_ViewRegTool(vv, ops, lo);
			mAction = AG_MenuAction(mi, ops->name,
			    ops->icon ? ops->icon->s : NULL,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, lo);
			AG_MenuSetIntBoolMp(mAction, &tool->selected, 0,
			    &OBJECT(vv)->lock);
		}
		
		/* Register (but hide) the special "insert package" tool. */
		VG_ViewRegTool(vv, &esPackageInsertTool, lo);

		AG_MenuToolbar(mi, NULL);
	}
	
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 85, 85);
	return (win);
}
