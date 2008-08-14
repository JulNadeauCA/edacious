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

#include "core.h"

static VG_ToolOps *ToolsCkt[] = {
	&esSchemSelectTool,
	&esSchemPortTool,
	NULL
};

static VG_ToolOps *ToolsVG[] = {
	&vgPointTool,
	&vgLineTool,
	&vgCircleTool,
	&vgTextTool,
	&vgPolygonTool,
#ifdef DEBUG
	&vgProximityTool,
#endif
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

	VG_Clear(scm->vg);
}

static void
Destroy(void *obj)
{
	ES_Schem *scm = obj;

	VG_Destroy(scm->vg);
	free(scm->vg);
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
	AG_WidgetFocus(vv);
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_TR, 40, 30);
	AG_WindowAttach(pwin, win);
	AG_WindowShow(win);
}

static void
SetSnappingMode(AG_Event *event)
{
	VG_View *vv = AG_PTR(1);
	int snapMode = AG_INT(2);
	
	VG_ViewSetSnapMode(vv, snapMode);
}

static void
FindSchemNodes(AG_Tlist *tl, VG_Node *node, int depth)
{
	AG_TlistItem *it;
	VG_Node *cNode;

	it = AG_TlistAdd(tl, NULL, "%s%u",
	    node->ops->name, node->handle);
	it->depth = depth;
	it->p1 = node;
	it->selected = (node->flags & VG_NODE_SELECTED);

	if (!TAILQ_EMPTY(&node->cNodes)) {
		it->flags |= AG_TLIST_HAS_CHILDREN;
	}
	if ((it->flags & AG_TLIST_HAS_CHILDREN) &&
	    AG_TlistVisibleChildren(tl, it)) {
		TAILQ_FOREACH(cNode, &node->cNodes, tree)
			FindSchemNodes(tl, cNode, depth+1);
	}
}

static void
PollSchemNodes(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Schem *scm = AG_PTR(1);

	AG_TlistBegin(tl);
	FindSchemNodes(tl, (VG_Node *)scm->vg->root, 0);
	AG_TlistEnd(tl);
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
	AG_Box *tlBox, *tbBox;
	AG_Pane *hPane;
	AG_Tlist *tlItems;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Component schematic: %s"),
	    OBJECT(scm)->name);
	
	vv = VG_ViewNew(NULL, vg, VG_VIEW_EXPAND|VG_VIEW_GRID|
	                          VG_VIEW_CONSTRUCTION);
	VG_ViewSetSnapMode(vv, VG_GRID);
	VG_ViewSetScale(vv, 2);
	VG_ViewSetGrid(vv, 0, VG_GRID_POINTS, 2, VG_GetColorRGB(100,100,100));
	VG_ViewSetGrid(vv, 1, VG_GRID_POINTS, 8, VG_GetColorRGB(200,200,0));

	menu = AG_MenuNew(win, AG_MENU_HFILL);
	tbRight = AG_ToolbarNew(NULL, AG_TOOLBAR_VERT, 1, 0);
	AG_ExpandVert(tbRight);

	mi = AG_MenuAddItem(menu, _("File"));
	{
		AG_MenuActionKb(mi, _("Close document"), agIconClose.s,
		    SDLK_w, KMOD_CTRL,
		    AGWINCLOSE(win));
	}
	mi = AG_MenuAddItem(menu, _("Edit"));
	{
		miSub = AG_MenuNode(mi, _("Snapping mode"), NULL);
		{
			AG_MenuToolbar(miSub, tbRight);
			AG_MenuActionKb(miSub, _("No snapping"),
			    vgIconSnapFree.s, SDLK_f, KMOD_CTRL,
			    SetSnappingMode, "%p,%i", vv, VG_FREE_POSITIONING);
			AG_MenuActionKb(miSub, _("Snap to grid"),
			    vgIconSnapGrid.s, SDLK_g, KMOD_CTRL,
			    SetSnappingMode, "%p,%i", vv, VG_GRID);
			AG_MenuToolbar(miSub, NULL);
			AG_ToolbarSeparator(tbRight);
		}
	}
	mi = AG_MenuAddItem(menu, _("Tools"));
	{
		VG_ToolOps **pOps, *ops;
		VG_Tool *tool;
	
		AG_MenuToolbar(mi, tbRight);
		for (pOps = &ToolsCkt[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			tool = VG_ViewRegTool(vv, ops, scm);
			AG_MenuAction(mi, ops->name,
			    ops->icon ? ops->icon->s : NULL,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, scm);
		}
		AG_MenuSeparator(mi);
		for (pOps = &ToolsVG[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			tool = VG_ViewRegTool(vv, ops, scm->vg);
			AG_MenuAction(mi, ops->name,
			    ops->icon ? ops->icon->s : NULL,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, scm->vg);
		}
		AG_MenuToolbar(mi, NULL);
	}
	mi = AG_MenuAddItem(menu, _("View"));
	{
		AG_MenuActionKb(mi, _("New view..."), esIconComponent.s,
		    SDLK_v, KMOD_CTRL,
		    CreateView, "%p,%p", win, scm);

		AG_MenuSeparator(mi);

		AG_MenuFlags(mi, _("Show grid"),
		    vgIconSnapGrid.s,
		    &vv->flags, VG_VIEW_GRID, 0);
		AG_MenuFlags(mi, _("Show construction geometry"),
		    esIconConstructionGeometry.s,
		    &vv->flags, VG_VIEW_CONSTRUCTION, 0);
#ifdef DEBUG
		AG_MenuFlags(mi, _("Show extents"),
		    vgIconBlock.s,
		    &vv->flags, VG_VIEW_EXTENTS, 0);
#endif
	}

	hPane = AG_PaneNewHoriz(win, AG_PANE_EXPAND);
	AG_BoxSetType(hPane->div[1], AG_BOX_HORIZ);
	{
		tlBox = AG_BoxNewVert(hPane->div[0], AG_BOX_EXPAND);
		AG_BoxSetPadding(tlBox, 0);
		{
			tlItems = AG_TlistNewPolled(tlBox,
			    AG_TLIST_TREE|AG_TLIST_EXPAND|AG_TLIST_NOSELSTATE,
			    PollSchemNodes, "%p", scm);
			AG_TlistSizeHint(tlItems, "<Point1234>", 4);
		}
		
		AG_ObjectAttach(hPane->div[1], vv);
		AG_WidgetFocus(vv);

		tbBox = AG_BoxNewVert(hPane->div[1], AG_BOX_VFILL);
		AG_BoxSetSpacing(tbBox, 0);
		AG_BoxSetPadding(tbBox, 0);
		AG_ObjectAttach(tbBox, tbRight);
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
