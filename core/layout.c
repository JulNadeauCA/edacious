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
 * Component schematic. This is mainly a vector drawing with specific
 * functionality for ports and labels.
 */

#include "core.h"

ES_Layout *
ES_LayoutNew(void *parent)
{
	ES_Layout *lo;

	lo = AG_Malloc(sizeof(ES_Layout));
	AG_ObjectInit(lo, &esLayoutClass);
	AG_ObjectAttach(parent, lo);
	return (lo);
}

static void
Init(void *obj)
{
	ES_Layout *lo = obj;

	lo->vg = VG_New(0);
}

static void
Reinit(void *obj)
{
	ES_Layout *lo = obj;

	VG_Clear(lo->vg);
}

static void
Destroy(void *obj)
{
	ES_Layout *lo = obj;

	VG_Destroy(lo->vg);
	free(lo->vg);
}

static int
Load(void *obj, AG_DataSource *ds, const AG_Version *ver)
{
	ES_Layout *lo = obj;
	
	return VG_Load(lo->vg, ds);
}

static int
Save(void *obj, AG_DataSource *ds)
{
	ES_Layout *lo = obj;

	VG_Save(lo->vg, ds);
	return (0);
}

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

static void *
Edit(void *p)
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

		mi2 = AG_MenuNode(mi, _("Schematic"), esIconCircuit.s);
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
		AG_Box *vBox, *hBox;

		VG_AddEditArea(vv, hPane->div[0]);

		vBox = AG_BoxNewVert(hPane->div[1], AG_BOX_EXPAND);
		AG_BoxSetSpacing(vBox, 0);
		AG_BoxSetPadding(vBox, 0);

		hBox = AG_BoxNewHoriz(vBox, AG_BOX_EXPAND);
		AG_BoxSetSpacing(hBox, 0);
		AG_BoxSetPadding(hBox, 0);

		AG_ObjectAttach(hBox, vv);
		AG_ObjectAttach(hBox, tbRight);
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
			tool = VG_ViewRegTool(vv, ops, NULL);
			mAction = AG_MenuAction(mi, ops->name,
			    ops->icon ? ops->icon->s : NULL,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, NULL);
			AG_MenuSetIntBoolMp(mAction, &tool->selected, 0,
			    &OBJECT(vv)->lock);
		}

		AG_MenuToolbar(mi, NULL);
	}
	
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 85, 85);
	return (win);
}

AG_ObjectClass esLayoutClass = {
	"Edacious(Layout)",
	sizeof(ES_Layout),
	{ 0,0 },
	Init,
	Reinit,
	Destroy,
	Load,
	Save,
	Edit
};
