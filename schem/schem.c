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
 * Component schematic. This is mainly a vector drawing with specific
 * functionality for ports and labels.
 */

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>

#include <eda.h>
#include "schem.h"

extern VG_ToolOps esSchemSelectTool;
extern VG_ToolOps esSchemPointTool;
extern VG_ToolOps esSchemLineTool;
extern VG_ToolOps esSchemCircleTool;
#ifdef DEBUG
extern VG_ToolOps esSchemProximityTool;
#endif

static VG_ToolOps *toolOps[] = {
	&esSchemSelectTool,
#ifdef DEBUG
	&esSchemProximityTool,
#endif
	&esSchemPointTool,
	&esSchemLineTool,
	&esSchemCircleTool,
	NULL
};

ES_Schem *
ES_SchemNew(void *parent)
{
	ES_Schem *scm;

	scm = AG_Malloc(sizeof(ES_Schem));
	AG_ObjectInit(scm, &esSchemClass);
	AG_ObjectAttach(parent, scm);
	return (scm);
}

static void
Init(void *obj)
{
	ES_Schem *scm = obj;

	scm->vg = VG_New(0);
}

static void
Reinit(void *obj)
{
	ES_Schem *scm = obj;

	VG_Reinit(scm->vg);
}

static void
Destroy(void *obj)
{
	ES_Schem *scm = obj;

	VG_Destroy(scm->vg);
}

static int
Load(void *obj, AG_DataSource *ds, const AG_Version *ver)
{
	ES_Schem *scm = obj;
	
	return VG_Load(scm->vg, ds);
}

static int
Save(void *obj, AG_DataSource *ds)
{
	ES_Schem *scm = obj;

	VG_Save(scm->vg, ds);
	return (0);
}

static void
CreateView(AG_Event *event)
{
	AG_Window *pwin = AG_PTR(1);
	ES_Schem *scm = AG_PTR(2);
	VG_View *vv;
	AG_Window *win;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("View of %s"), OBJECT(scm)->name);

	vv = VG_ViewNew(win, scm->vg, VG_VIEW_EXPAND);
	VG_ViewSetScale(vv, 16.0f);
	VG_ViewSetScaleMin(vv, 10.0f);

	AG_WidgetFocus(vv);
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_TR, 60, 50);
	AG_WindowAttach(pwin, win);
	AG_WindowShow(win);
}

static void *
Edit(void *obj)
{
	ES_Schem *scm = obj;
	VG *vg = scm->vg;
	VG_View *vv;
	AG_Window *win;
	AG_Toolbar *tbRight;
	AG_Menu *menu;
	AG_MenuItem *mi, *miSub;
	AG_Box *box;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Component schematic: %s"),
	    OBJECT(scm)->name);
	
	menu = AG_MenuNew(win, AG_MENU_HFILL);
	tbRight = AG_ToolbarNew(NULL, AG_TOOLBAR_VERT, 1, 0);
	vv = VG_ViewNew(NULL, vg, VG_VIEW_EXPAND|VG_VIEW_GRID);
	VG_ViewSetSnapMode(vv, VG_GRID);
	VG_ViewSetScale(vv, 64.0f);
	VG_ViewSetScaleMin(vv, 10.0f);
	VG_ViewSetGridInterval(vv, 0.5f);
	
	mi = AG_MenuAddItem(menu, _("File"));
	{
		AG_MenuActionKb(mi, _("Close document"), agIconClose.s,
		    SDLK_w, KMOD_CTRL,
		    AGWINCLOSE(win));
	}
	mi = AG_MenuAddItem(menu, _("Edit"));
	{
		miSub = AG_MenuNode(mi, _("Snapping mode"), NULL);
		VG_SnapMenu(menu, miSub, vv);
	}
	mi = AG_MenuAddItem(menu, _("Tools"));
	{
		VG_ToolOps **pOps, *ops;
		VG_Tool *tool;
		
		for (pOps = &toolOps[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			tool = VG_ViewRegTool(vv, ops, scm);
			AG_MenuTool(mi, tbRight, ops->name,
			    ops->icon ? ops->icon->s : NULL, 0,0,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, scm);
		}
	}
	mi = AG_MenuAddItem(menu, _("View"));
	{
		AG_MenuActionKb(mi, _("New view..."), esIconComponent.s,
		    SDLK_v, KMOD_CTRL,
		    CreateView, "%p,%p", win, scm);
		AG_MenuSeparator(mi);
		AG_MenuUintFlags(mi, _("Show grid"), vgIconSnapGrid.s,
		    &vv->flags, VG_VIEW_GRID, 0);
#ifdef DEBUG
		AG_MenuUintFlags(mi, _("Show extents"), vgIconBlock.s,
		    &vv->flags, VG_VIEW_EXTENTS, 0);
#endif
	}

	box = AG_BoxNewHoriz(win, AG_BOX_EXPAND);
	{
		AG_ObjectAttach(box, vv);
		AG_ObjectAttach(box, tbRight);
		AG_WidgetFocus(vv);
	}

	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 70, 65);
	return (win);
}

AG_ObjectClass esSchemClass = {
	"ES_Schem",
	sizeof(ES_Schem),
	{ 0,0 },
	Init,
	Reinit,
	Destroy,
	Load,
	Save,
	Edit
};
