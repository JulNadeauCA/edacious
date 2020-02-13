/*
 * Copyright (c) 2008-2020 Julien Nadeau Carriere (vedge@csoft.net)
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
 * Editor for general-purpose schematic drawings.
 */

#include "core.h"

/* Create an alternate view. */
static void
CreateView(AG_Event *event)
{
	AG_Window *pwin = AG_WINDOW_PTR(1);
	ES_Schem *scm = ES_SCHEM_PTR(2);
	VG_View *vv;
	AG_Window *win;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("View of %s"), OBJECT(scm)->name);

	vv = VG_ViewNew(win, scm->vg, VG_VIEW_EXPAND);
	AG_WidgetFocus(vv);
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_TR, 60, 50);
	AG_WindowAttach(pwin, win);
	AG_WindowShow(win);
}

void *
ES_SchemEdit(void *p)
{
	ES_Schem *scm = p;
	AG_Window *win;
	AG_Toolbar *tbRight;
	AG_Pane *hPane;
	VG_View *vv;
	AG_Menu *menu;
	AG_MenuItem *m, *mSub;

	win = AG_WindowNew(0);

	tbRight = AG_ToolbarNew(NULL, AG_TOOLBAR_VERT, 1, 0);
	AG_ExpandVert(tbRight);
	
	vv = VG_ViewNew(NULL, scm->vg, VG_VIEW_EXPAND|VG_VIEW_GRID|
	                               VG_VIEW_CONSTRUCTION);
	VG_ViewSetSnapMode(vv, VG_GRID);
	VG_ViewSetScale(vv, 2);

	menu = AG_MenuNew(win, AG_MENU_HFILL);

	m = AG_MenuNode(menu->root, _("File"), NULL);
	{
		ES_FileMenu(m, scm);
	}
	m = AG_MenuNode(menu->root, _("Edit"), NULL);
	{
		mSub = AG_MenuNode(m, _("Snapping mode"), NULL);
		VG_SnapMenu(mSub, vv);
		AG_MenuSeparator(m);
		ES_EditMenu(m, scm);
	}
	m = AG_MenuNode(menu->root, _("View"), NULL);
	{
		AG_MenuActionKb(m, _("New view..."), esIconCircuit.s,
		    AG_KEY_V, AG_KEYMOD_CTRL,
		    CreateView, "%p,%p", win, scm);

		AG_MenuSeparator(m);

		mSub = AG_MenuNode(m, _("Schematic"), esIconCircuit.s);
		{
			AG_MenuFlags(mSub, _("Grid"), vgIconSnapGrid.s,
			    &vv->flags, VG_VIEW_GRID, 0);
			AG_MenuFlags(mSub, _("Construction geometry"),
			    esIconConstructionGeometry.s,
			    &vv->flags, VG_VIEW_CONSTRUCTION, 0);
		}
	}
	
	hPane = AG_PaneNewHoriz(win, AG_PANE_EXPAND);
	{
		AG_Box *vBox, *hBox;

		VG_AddEditArea(vv, hPane->div[0]);

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

	m = AG_MenuNode(menu->root, _("Tools"), NULL);
	{
		AG_MenuItem *mAction;
		VG_ToolOps **pOps, *ops;
		VG_Tool *tool;

		AG_MenuToolbar(m, tbRight);
		
		for (pOps = &esVgTools[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			tool = VG_ViewRegTool(vv, ops, scm);
			mAction = AG_MenuAction(m, ops->name,
			    ops->icon ? ops->icon->s : NULL,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, scm);
			AG_MenuSetIntBoolMp(mAction, &tool->selected, 0,
			    &OBJECT(vv)->lock);
			if (ops == &vgSelectTool) {
				VG_ViewSetDefaultTool(vv, tool);
				VG_ViewSelectTool(vv, tool, scm);
			}
		}

		AG_MenuToolbar(m, NULL);
	}

	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 50, 40);
	AG_PaneMoveDivider(hPane, 200);
	return (win);
}
