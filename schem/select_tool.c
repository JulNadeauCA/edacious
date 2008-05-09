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
 * Tool for selecting and moving entities in a component schematic.
 */

#include <eda.h>
#include <agar/core/limits.h>

typedef struct es_schem_select_tool {
	VG_Tool _inherit;
	int moving;
} ES_SchemSelectTool;

void *
VG_SchemFindPoint(VG_View *vv, VG_Vector vCurs, void *ignore)
{
	float prox, proxNearest = AG_FLT_MAX;
	VG_Node *vn, *vnNearest = NULL;
	VG_Vector v;

	TAILQ_FOREACH(vn, &vv->vg->nodes, list) {
		if (vn->ops->pointProximity == NULL ||
		    vn == ignore ||
		    !VG_NodeIsClass(vn, "Point")) {
			continue;
		}
		v = vCurs;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox < vv->gridIval) {
			if (prox < proxNearest) {
				proxNearest = prox;
				vnNearest = vn;
			}
		}
	}
	return (vnNearest);
}

void *
VG_SchemHighlightNearestPoint(VG_View *vv, ES_Schem *scm, VG_Vector vCurs,
    void *ignore)
{
	float prox, proxNearest = AG_FLT_MAX;
	VG_Node *vn, *vnNearest = NULL;
	VG_Vector v;

	TAILQ_FOREACH(vn, &scm->vg->nodes, list) {
		vn->flags &= ~(VG_NODE_MOUSEOVER);
		if (vn->ops->pointProximity == NULL ||
		    vn == ignore ||
		    !VG_NodeIsClass(vn, "Point")) {
			continue;
		}
		v = vCurs;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox < vv->gridIval) {
			if (prox < proxNearest) {
				proxNearest = prox;
				vnNearest = vn;
			}
		}
	}
	return (vnNearest);
}

void *
VG_SchemSelectNearest(VG_View *vv, ES_Schem *scm, VG_Vector vCurs)
{
	float prox, proxNearest;
	VG_Node *vn, *vnNearest;
	VG_Vector v;
	int multi = SDL_GetModState() & KMOD_CTRL;

	/* Always prioritize points at a fixed distance. */
	proxNearest = AG_FLT_MAX;
	vnNearest = NULL;
	TAILQ_FOREACH(vn, &scm->vg->nodes, list) {
		if (vn->ops->pointProximity == NULL) {
			continue;
		}
		if (!multi) {
			vn->flags &= ~(VG_NODE_SELECTED);
		}
		if (!VG_NodeIsClass(vn, "Point")) {
			continue;
		}
		v = vCurs;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox <= 0.25f) {
			if (prox < proxNearest) {
				proxNearest = prox;
				vnNearest = vn;
			}
		}
	}
	if (vnNearest != NULL) {
		vnNearest->flags |= VG_NODE_SELECTED;
		return (vnNearest);
	}

	/* No point is near, perform a proper query. */
	proxNearest = AG_FLT_MAX;
	vnNearest = NULL;
	TAILQ_FOREACH(vn, &scm->vg->nodes, list) {
		if (vn->ops->pointProximity == NULL) {
			continue;
		}
		if (!multi) {
			vn->flags &= ~(VG_NODE_SELECTED);
		}
		v = vCurs;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox < proxNearest) {
			proxNearest = prox;
			vnNearest = vn;
		}
	}
	if (vnNearest != NULL) {
		vnNearest->flags |= VG_NODE_SELECTED;
	}
	return (vnNearest);
}

void *
VG_SchemHighlightNearest(VG_View *vv, ES_Schem *scm, VG_Vector vCurs)
{
	float prox, proxNearest;
	VG_Node *vn, *vnNearest;
	VG_Vector v;

	/* Always prioritize points at a fixed distance. */
	proxNearest = AG_FLT_MAX;
	vnNearest = NULL;
	TAILQ_FOREACH(vn, &scm->vg->nodes, list) {
		if (vn->ops->pointProximity == NULL) {
			continue;
		}
		vn->flags &= ~(VG_NODE_MOUSEOVER);
		if (!VG_NodeIsClass(vn, "Point")) {
			continue;
		}
		v = vCurs;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox <= 0.25f) {
			if (prox < proxNearest) {
				proxNearest = prox;
				vnNearest = vn;
			}
		}
	}
	if (vnNearest != NULL) {
		vnNearest->flags |= VG_NODE_MOUSEOVER;
		return (vnNearest);
	}

	/* No point is near, perform a proper query. */
	proxNearest = AG_FLT_MAX;
	vnNearest = NULL;
	TAILQ_FOREACH(vn, &scm->vg->nodes, list) {
		if (vn->ops->pointProximity == NULL) {
			continue;
		}
		vn->flags &= ~(VG_NODE_MOUSEOVER);
		v = vCurs;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox < proxNearest) {
			proxNearest = prox;
			vnNearest = vn;
		}
	}
	if (vnNearest != NULL) {
		vnNearest->flags |= VG_NODE_MOUSEOVER;
	}
	return (vnNearest);
}

static int
MouseButtonDown(void *p, VG_Vector v, int b)
{
	ES_SchemSelectTool *t = p;
	ES_Schem *scm = VGTOOL(t)->p;
	VG_View *vv = VGTOOL(t)->vgv;

	if (b != SDL_BUTTON_LEFT) {
		return (0);
	}
	VG_SchemSelectNearest(vv, scm, v);
	t->moving = 1;
	return (1);
}

static int
MouseButtonUp(void *p, VG_Vector v, int b)
{
	ES_SchemSelectTool *t = p;

	if (b != SDL_BUTTON_LEFT) {
		return (0);
	}
	t->moving = 0;
	return (1);
}

static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int buttons)
{
	ES_SchemSelectTool *t = p;
	ES_Schem *scm = VGTOOL(t)->p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Node *vn;
	VG_Vector v;

	if (t->moving) {
		v = vPos;

		if (!(SDL_GetModState() & (KMOD_SHIFT|KMOD_CTRL))) {
			VG_ApplyConstraints(vv, &v);
		}
		TAILQ_FOREACH(vn, &scm->vg->nodes, list) {
			if (!(vn->flags & VG_NODE_SELECTED)) {
				continue;
			}
			if (vn->ops->moveNode != NULL)
				vn->ops->moveNode(vn, v, vRel);
		}
		return (1);
	}
	VG_SchemHighlightNearest(vv, scm, vPos);
	return (0);
}

static int
KeyDown(void *p, int ksym, int kmod, int unicode)
{
	ES_SchemSelectTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Node *vn;
	Uint nDel = 0;

	if (ksym == SDLK_DELETE || ksym == SDLK_BACKSPACE) {
del:
		TAILQ_FOREACH(vn, &vv->vg->nodes, list) {
			if (vn->flags & VG_NODE_SELECTED) {
				if (VG_Delete(vn) == -1) {
					vn->flags &= ~(VG_NODE_SELECTED);
					VG_Status(vv, "%s", AG_GetError());
				} else {
					nDel++;
				}
				goto del;
			}
		}
		VG_Status(vv, _("Deleted %u entities"), nDel);
		return (1);
	}
	return (0);
}

static void
Init(void *p)
{
	ES_SchemSelectTool *t = p;

	t->moving = 0;
}

VG_ToolOps esSchemSelectTool = {
	N_("Select"),
	N_("Select and move entities in the schematic."),
	&esIconSelectArrow,
	sizeof(ES_SchemSelectTool),
	VG_NOSNAP,
	Init,
	NULL,			/* destroy */
	NULL,			/* edit */
	NULL,			/* predraw */
	NULL,			/* postdraw */
	MouseMotion,
	MouseButtonDown,
	MouseButtonUp,
	KeyDown,
	NULL			/* keyup */
};
