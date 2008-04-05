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
 * Circle tool.
 */

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>

#include <eda.h>
#include "schem.h"

typedef struct es_schem_circle_tool {
	VG_Tool _inherit;
	VG_Node *curCircle;
} ES_SchemCircleTool;

static void
Init(void *p)
{
	ES_SchemCircleTool *t = p;

	t->curCircle = NULL;
}

static __inline__ void
AdjustRadius(VG_View *vv, VG_Node *vn, float x, float y)
{
	VG_Select(vv->vg, vn);
	VG_CircleRadius(vv->vg, VG_Distance2(x,y, vn->vtx[0].x,vn->vtx[0].y));
	VG_End(vv->vg);
	VG_Status(vv, _("Circle radius: %.2f"),
	    vn->vg_args.vg_circle.radius);
}

static int
MouseButtonDown(void *p, float x, float y, int b)
{
	ES_SchemCircleTool *t = p;
	ES_Schem *scm = VGTOOL(t)->p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG *vg = scm->vg;

	switch (b) {
	case SDL_BUTTON_LEFT:
		if (t->curCircle == NULL) {
			VG_Status(vv, _("New circle at %.2f,%.2f"), x, y);
			t->curCircle = VG_Begin(vg, VG_CIRCLE);
			VG_Vertex2(vg, x, y);
			VG_End(vg);
		} else {
			AdjustRadius(vv, t->curCircle, x, y);
			t->curCircle = NULL;
		}
		return (1);
	case SDL_BUTTON_MIDDLE:
	case SDL_BUTTON_RIGHT:
		if (t->curCircle != NULL) {
			VG_Delete(vg, t->curCircle);
			t->curCircle = NULL;
		}
		return (1);
	default:
		return (0);
	}
}

static int
MouseMotion(void *p, float x, float y, float xrel, float yrel, int b)
{
	ES_SchemCircleTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	
	if (t->curCircle != NULL) {
		AdjustRadius(vv, t->curCircle, x, y);
	}
	return (0);
}

VG_ToolOps esSchemCircleTool = {
	N_("Circle"),
	N_("Insert circles in the component schematic."),
	&vgIconCircle,
	sizeof(ES_SchemCircleTool),
	0,
	Init,
	NULL,			/* destroy */
	NULL,			/* edit */
	NULL,			/* predraw */
	NULL,			/* postdraw */
	MouseMotion,
	MouseButtonDown,
	NULL,			/* mousebuttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};
