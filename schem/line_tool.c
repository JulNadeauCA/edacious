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
 * Line tool.
 */

#include <eda.h>

typedef struct es_schem_line_tool {
	VG_Tool _inherit;
	VG_Line *vlCur;
} ES_SchemLineTool;

static void
Init(void *p)
{
	ES_SchemLineTool *t = p;

	t->vlCur = NULL;
}

static int
MouseButtonDown(void *p, VG_Vector vPos, int button)
{
	ES_SchemLineTool *t = p;
	ES_Schem *scm = VGTOOL(t)->p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG *vg = scm->vg;
	VG_Point *p1, *p2;

	switch (button) {
	case SDL_BUTTON_LEFT:
		if (t->vlCur == NULL) {
			if (!(p1 = VG_SchemFindPoint(vv, vPos, NULL))) {
				p1 = VG_PointNew(vg->root, vPos);
			}
			p2 = VG_PointNew(vg->root, vPos);
			t->vlCur = VG_LineNew(vg->root, p1, p2);
		} else {
			if ((p2 = VG_SchemFindPoint(vv, vPos, t->vlCur->p2))) {
				VG_DelRef(t->vlCur, t->vlCur->p2);
				VG_Delete(t->vlCur->p2);
				t->vlCur->p2 = p2;
				VG_AddRef(t->vlCur, p2);
			} else {
				VG_SetPosition(t->vlCur->p2, vPos);
			}
			t->vlCur = NULL;
		}
		return (1);
	case SDL_BUTTON_MIDDLE:
	case SDL_BUTTON_RIGHT:
		if (t->vlCur != NULL) {
			VG_Delete(t->vlCur);
			t->vlCur = NULL;
		}
		return (1);
	default:
		return (0);
	}
}

static void
PostDraw(void *p, VG_View *vv)
{
	ES_SchemLineTool *t = p;
	int x, y;

	VG_GetViewCoords(vv, VGTOOL(t)->vCursor, &x,&y);
	AG_DrawCircle(vv, x,y, 3, VG_MapColorRGB(vv->vg->selectionColor));
}

static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int b)
{
	ES_SchemLineTool *t = p;
	ES_Schem *scm = VGTOOL(t)->p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Point *pEx;
	VG_Vector pos;
	float theta, rad;
	
	if (t->vlCur != NULL) {
		pEx = t->vlCur->p1;
		pos = VG_Pos(pEx);
		theta = VG_Atan2(vPos.y - pos.y,
		                 vPos.x - pos.x);
		rad = VG_Hypot(vPos.x - pos.x,
		               vPos.y - pos.y);
		if ((pEx = VG_SchemFindPoint(vv, vPos, t->vlCur->p2))) {
			VG_Status(vv, _("Use Point%u"), VGNODE(pEx)->handle);
		} else {
			VG_Status(vv,
			    _("Create Point at %.2f,%.2f (%.2f|%.2f\xc2\xb0)"),
			    vPos.x, vPos.y, rad, VG_Degrees(theta));
		}
		VG_SetPosition(t->vlCur->p2, vPos);
	} else {
		if ((pEx = VG_SchemFindPoint(vv, vPos, NULL))) {
			VG_Status(vv, _("Start Line at Point%u"),
			    VGNODE(pEx)->handle);
		} else {
			VG_Status(vv, _("Start Line at %.2f,%.2f"), vPos.x,
			    vPos.y);
		}
	}
	return (0);
}

VG_ToolOps esSchemLineTool = {
	N_("Line"),
	N_("Insert lines in the component schematic."),
	&vgIconLine,
	sizeof(ES_SchemLineTool),
	0,
	Init,
	NULL,			/* destroy */
	NULL,			/* edit */
	NULL,			/* predraw */
	PostDraw,
	MouseMotion,
	MouseButtonDown,
	NULL,			/* mousebuttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};
