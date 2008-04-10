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
 * Connection point in the schematic.
 */

#include <eda.h>
#include <agar/gui/primitive.h>
#include <agar/core/limits.h>

static void
Init(void *p)
{
	ES_SchemPort *sp = p;

	sp->p = NULL;
	sp->lbl = NULL;
	sp->n = 0;
	sp->sym[0] = '\0';
	sp->r = 0.0625f;
}

static int
Load(void *p, AG_DataSource *ds, const AG_Version *ver)
{
	ES_SchemPort *sp = p;

	sp->p = VG_ReadRef(ds, sp, "Point");
	sp->lbl = VG_ReadRef(ds, sp, "Text");
	sp->n = (Uint)AG_ReadUint32(ds);
	AG_CopyString(sp->sym, ds, sizeof(sp->sym));
	return (0);
}

static void
Save(void *p, AG_DataSource *ds)
{
	ES_SchemPort *sp = p;

	VG_WriteRef(ds, sp->p);
	VG_WriteRef(ds, sp->lbl);
	AG_WriteUint32(ds, (Uint32)sp->n);
	AG_WriteString(ds, sp->sym);
}

static void
Draw(void *p, VG_View *vv)
{
	ES_SchemPort *sp = p;
	VG_Vector vPos = VG_PointPos(sp->p);
	int x, y;
	
	VG_GetViewCoords(vv, vPos, &x, &y);
	AG_DrawCircle(vv, x, y, (int)(sp->r*vv->scale),
	    VG_MapColorRGB(VGNODE(sp)->color));
}

static void
Extent(void *p, VG_View *vv, VG_Rect *r)
{
	ES_SchemPort *sp = p;
	VG_Vector vPos = VG_PointPos(sp->p);

	r->x = vPos.x - sp->r;
	r->y = vPos.y - sp->r;
	r->w = sp->r*2.0f;
	r->h = sp->r*2.0f;
}

static float
PointProximity(void *p, VG_Vector *vPt)
{
	ES_SchemPort *sp = p;
	float d;

	d = VG_Distance(VG_PointPos(sp->p), *vPt);
	vPt->x = sp->p->x;
	vPt->y = sp->p->y;
	return (d);
}

static void
Delete(void *p)
{
	ES_SchemPort *sp = p;

	if (VG_DelRef(sp, sp->p) == 0)
		VG_Delete(sp->p);
	if (VG_DelRef(sp, sp->lbl) == 0)
		VG_Delete(sp->lbl);
}

static void
Move(void *p, VG_Vector vCurs, VG_Vector vRel)
{
	ES_SchemPort *sp = p;

	sp->r = VG_Distance(VG_PointPos(sp->p), vCurs);
}

const VG_NodeOps esSchemPortOps = {
	N_("SchemPort"),
	&esIconPortEditor,
	sizeof(ES_SchemPort),
	Init,
	NULL,			/* destroy */
	Load,
	Save,
	Draw,
	Extent,
	PointProximity,
	NULL,			/* lineProximity */
	Delete,
	Move
};

/****************/
/* Edition tool */
/****************/

typedef struct es_schem_select_tool {
	VG_Tool _inherit;
} ES_SchemPortTool;

static int
MouseButtonDown(void *p, VG_Vector vPos, int button)
{
	ES_SchemPortTool *t = p;
	ES_Schem *scm = VGTOOL(t)->p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Point *vp;
	ES_SchemPort *sp;

	if (button != SDL_BUTTON_LEFT) {
		return (0);
	}
	if ((vp = VG_SchemFindPoint(scm, vPos, NULL)) != NULL) {
//		sp = ES_SchemPortNew(scm->vg->root, vp);
		VG_Status(vv, _("Not yet"));
	} else {
		VG_Status(vv, _("Must select a point to create a port"));
	}
	return (1);
}

static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int buttons)
{
	ES_SchemPortTool *t = p;
	ES_Schem *scm = VGTOOL(t)->p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Point *vp;

	if ((vp = VG_SchemHighlightNearestPoint(scm, vPos, NULL)) != NULL) {
		VG_Status(vv, _("Create port on Point%u"), VGNODE(vp)->handle);
	} else {
		VG_Status(vv, _("Select a Point"));
	}
	return (0);
}

static int
KeyDown(void *p, int ksym, int kmod, int unicode)
{
	ES_SchemPortTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Node *vn;
	Uint nDel = 0;

	if (ksym == SDLK_DELETE || ksym == SDLK_BACKSPACE) {
del:
		TAILQ_FOREACH(vn, &vv->vg->nodes, list) {
			if (!VG_NodeIsClass(vn, "SchemPort") ||
			    !(vn->flags & VG_NODE_SELECTED)) {
				continue;
			}
			if (VG_Delete(vn) == -1) {
				vn->flags &= ~(VG_NODE_SELECTED);
				VG_Status(vv, "%s", AG_GetError());
			} else {
				nDel++;
			}
			goto del;
		}
		VG_Status(vv, _("Deleted %u ports"), nDel);
		return (1);
	}
	return (0);
}

VG_ToolOps esSchemPortTool = {
	N_("Port Editor"),
	N_("Create connection points for the schematic."),
	&esIconPortEditor,
	sizeof(ES_SchemPortTool),
	VG_NOSNAP,
	NULL,
	NULL,			/* destroy */
	NULL,			/* edit */
	NULL,			/* predraw */
	NULL,			/* postdraw */
	MouseMotion,
	MouseButtonDown,
	NULL,			/* mousebuttonup */
	KeyDown,
	NULL			/* keyup */
};
