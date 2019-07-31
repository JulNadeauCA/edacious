/*
 * Copyright (c) 2009-2019 Julien Nadeau Carriere <vedge@csoft.net>
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
 * Layout Trace tool. Insert a conductive line in a PCB layout.
 */

#include "core.h"

typedef struct es_layout_trace_tool {
	VG_Tool _inherit;
	Uint flags;
#define CREATE_NEW_PORTS 0x01		/* Allow creation of new connection points */
	ES_LayoutTrace *curTrace;	/* Trace being created */
} ES_LayoutTraceTool;

static void
Init(void *p)
{
	ES_LayoutTraceTool *t = p;

	t->flags = 0;
	t->curTrace = NULL;
}

static void
UpdateStatus(VG_View *vv, ES_LayoutNode *lp)
{
	if (lp == NULL) {
		VG_Status(vv, _("Start new trace"));
	} else {
		VG_Status(vv, _("Connect trace to %s%u"),
		    VGNODE(lp)->ops->name, VGNODE(lp)->handle);
	}
}

static int
MouseButtonDown(void *p, VG_Vector vPos, int button)
{
	ES_LayoutTraceTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG *vg = vv->vg;
	ES_LayoutNode *lp1, *lp2;
	
	switch (button) {
	case AG_MOUSE_LEFT:
		if (t->curTrace == NULL) {
			lp1 = ES_LayoutNodeNew(vg->root);
			lp2 = ES_LayoutNodeNew(vg->root);
			VG_Translate(lp1, vPos);
			VG_Translate(lp2, vPos);
			t->curTrace = ES_LayoutTraceNew(vg->root, lp1, lp2);
		} else {
			t->curTrace = NULL;
		}
		return (1);
	case AG_MOUSE_RIGHT:
		if (t->curTrace != NULL) {
			VG_NodeDetach(t->curTrace);
			VG_NodeDestroy(t->curTrace);
			t->curTrace = NULL;
			return (1);
		}
		return (0);
	default:
		return (0);
	}
}

static int
MouseButtonUp(void *p, VG_Vector vPos, int button)
{
	ES_LayoutTraceTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	ES_LayoutNode *spNear;

	switch (button) {
	case AG_MOUSE_LEFT:
		spNear = VG_PointProximityMax(vv, "LayoutNode", &vPos, NULL,
		    t->curTrace, vv->pointSelRadius);
		UpdateStatus(vv, spNear);
		return (0);
	default:
		return (0);
	}
}

static void
PostDraw(void *p, VG_View *vv)
{
	VG_Tool *t = p;
	AG_Color c;
	int x, y;

	VG_GetViewCoords(vv, t->vCursor, &x,&y);
	c = VG_MapColorRGB(vv->vg->selectionColor);
	AG_DrawCircle(vv, x,y, 3, &c);
}

static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int buttons)
{
	ES_LayoutTraceTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	ES_LayoutNode *spNear;

	spNear = VG_PointProximityMax(vv, "LayoutNode", &vPos, NULL,
	    NULL, vv->pointSelRadius);
	if (t->curTrace != NULL) {
		VG_SetPosition(t->curTrace->p2,
		    (spNear != NULL) ? VG_Pos(spNear) : vPos);
	}
	UpdateStatus(vv, spNear);
	return (0);
}

static void *
Edit(void *p, VG_View *vv)
{
	ES_LayoutTraceTool *t = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	AG_CheckboxNewFlag(box, 0, _("Create new connection points"),
	    &t->flags, CREATE_NEW_PORTS);
	return (box);
}

VG_ToolOps esLayoutTraceTool = {
	N_("Insert trace"),
	N_("Create a conductive trace."),
	&esIconInsertWire,
	sizeof(ES_LayoutTraceTool),
	0,
	Init,
	NULL,			/* destroy */
	Edit,
	NULL,			/* predraw */
	PostDraw,
	NULL,			/* selected */
	NULL,			/* deselected */
	MouseMotion,
	MouseButtonDown,
	MouseButtonUp,
	NULL,			/* keydown */
	NULL			/* keyup */
};
