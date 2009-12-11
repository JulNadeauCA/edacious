/*
 * Copyright (c) 2008-2009 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Layout editor: Tool for selecting and moving entities in a circuit layout.
 */

#include "core.h"
#include <agar/core/limits.h>

typedef struct es_layout_select_tool {
	VG_Tool _inherit;
	Uint flags;
#define MOVING_ENTITIES	0x01	/* Translation is in progress */
	VG_Vector vLast;	/* For grid snapping */
	VG_Node *vnMouseOver;	/* Element under cursor */
} ES_LayoutSelectTool;

/* Return the entity nearest to vPos. */
void *
ES_LayoutNearest(VG_View *vv, VG_Vector vPos)
{
	VG *vg = vv->vg;
	float prox, proxNearest;
	VG_Node *vn, *vnNearest;
	VG_Vector v;

	/* First check if we intersect a block. */
	TAILQ_FOREACH(vn, &vg->nodes, list) {
		if (!VG_NodeIsClass(vn, "LayoutBlock")) {
			continue;
		}
		v = vPos;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox == 0.0f)
			return (vn);
	}

	/* Then prioritize points at a fixed distance. */
	proxNearest = AG_FLT_MAX;
	vnNearest = NULL;
	TAILQ_FOREACH(vn, &vg->nodes, list) {
		if (!VG_NodeIsClass(vn, "Point")) {
			continue;
		}
		v = vPos;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox <= vv->pointSelRadius) {
			if (prox < proxNearest) {
				proxNearest = prox;
				vnNearest = vn;
			}
		}
	}
	if (vnNearest != NULL)
		return (vnNearest);

	/* Finally, fallback to a general query. */
	proxNearest = AG_FLT_MAX;
	vnNearest = NULL;
	TAILQ_FOREACH(vn, &vg->nodes, list) {
		if (vn->ops->pointProximity == NULL) {
			continue;
		}
		v = vPos;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox < proxNearest) {
			proxNearest = prox;
			vnNearest = vn;
		}
	}
	return (vnNearest);
}

static int
MouseButtonDown(void *p, VG_Vector v, int b)
{
	ES_LayoutSelectTool *t = p;
	ES_LayoutBlock *lb;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Node *vn;

	if ((vn = ES_LayoutNearest(vv, v)) == NULL) {
		return (0);
	}
	switch (b) {
	case AG_MOUSE_LEFT:
		t->vLast = v;
		if (VG_SELECT_MULTI(vv)) {
			if (vn->flags & VG_NODE_SELECTED) {
				vn->flags &= ~(VG_NODE_SELECTED);
			} else {
				vn->flags |= VG_NODE_SELECTED;
			
				if (VG_NodeIsClass(vn, "LayoutBlock")) {
					lb = (ES_LayoutBlock *)vn;
					ES_SelectComponent(lb->com, vv);
				} else {
					VG_EditNode(vv, 0, vn);
				}
			}
		} else {
			if (VG_NodeIsClass(vn, "LayoutBlock")) {
				lb = (ES_LayoutBlock *)vn;
				ES_UnselectAllComponents(COMCIRCUIT(lb->com), vv);
				ES_SelectComponent(lb->com, vv);
			} else {
				VG_EditNode(vv, 0, vn);
			}
			VG_UnselectAll(vv->vg);
			vn->flags |= VG_NODE_SELECTED;
		}
		t->flags |= MOVING_ENTITIES;
		return (1);
	default:
		return (0);
	}
}

static int
MouseButtonUp(void *p, VG_Vector v, int b)
{
	ES_LayoutSelectTool *t = p;

	if (b != AG_MOUSE_LEFT) {
		return (0);
	}
	t->flags &= ~(MOVING_ENTITIES);
	return (1);
}

static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int buttons)
{
	ES_LayoutSelectTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Node *vn;
	VG_Vector v;

	/* Provide visual feedback of current selection. */
	if ((t->flags & MOVING_ENTITIES) == 0) {
		if ((vn = ES_LayoutNearest(vv, vPos)) != NULL &&
		    t->vnMouseOver != vn) {
			t->vnMouseOver = vn;
			if (VG_NodeIsClass(vn, "LayoutBlock")) {
				ES_HighlightComponent(SCHEM_BLOCK(vn)->com);
				VG_Status(vv, _("Select component: %s"),
				    OBJECT(SCHEM_BLOCK(vn)->com)->name);
			} else if (VG_NodeIsClass(vn, "LayoutTrace")) {
				VG_Status(vv, _("Select trace #%d"), vn->handle);
			} else if (VG_NodeIsClass(vn, "LayoutHole")) {
				VG_Status(vv, _("Select hole #%d"), vn->handle);
			} else {
				VG_Status(vv, _("Select layout entity: %s%d"),
				    vn->ops->name, vn->handle);
			}
		}
		return (0);
	}

	/* Translate any selected entities. */
	v = vPos;
	if (!VG_SKIP_CONSTRAINTS(vv)) {
		VG_Vector vLast, vSnapRel;

		vLast = t->vLast;
		VG_ApplyConstraints(vv, &v);
		VG_ApplyConstraints(vv, &vLast);
		vSnapRel.x = v.x - vLast.x;
		vSnapRel.y = v.y - vLast.y;

		if (vSnapRel.x != 0.0 || vSnapRel.y != 0.0) {
			TAILQ_FOREACH(vn, &vv->vg->nodes, list) {
				if (!(vn->flags & VG_NODE_SELECTED) ||
				    vn->ops->moveNode == NULL) {
					continue;
				}
				vn->ops->moveNode(vn, v, vSnapRel);
				VG_Status(vv, _("Moving entity: %s%d (grid)"),
				    vn->ops->name, vn->handle);
			}
			t->vLast = v;
		}
	} else {
		TAILQ_FOREACH(vn, &vv->vg->nodes, list) {
			if (!(vn->flags & VG_NODE_SELECTED) ||
			    vn->ops->moveNode == NULL) {
				continue;
			}
			vn->ops->moveNode(vn, v, vRel);
			VG_Status(vv, _("Moving entity: %s%d (free)"),
			    vn->ops->name, vn->handle);
		}
	}
	return (0);
}

static int
KeyDown(void *p, int ksym, int kmod, Uint32 unicode)
{
	ES_LayoutSelectTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Node *vn;
	Uint nDel = 0;

	if (ksym == AG_KEY_DELETE || ksym == AG_KEY_BACKSPACE) {
del:
		TAILQ_FOREACH(vn, &vv->vg->nodes, list) {
			if (vn->flags & VG_NODE_SELECTED) {
				VG_ClearEditAreas(vv);
				if (VG_Delete(vn) == -1) {
					vn->flags &= ~(VG_NODE_SELECTED);
					VG_Status(vv, "%s%u: %s",
					    vn->ops->name, vn->handle,
					    AG_GetError());
					return (0);
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
PostDraw(void *p, VG_View *vv)
{
	ES_LayoutSelectTool *t = p;
	int x, y;

	if (t->vnMouseOver != NULL) {
		VG_GetViewCoords(vv, VG_Pos(t->vnMouseOver), &x,&y);
		AG_DrawCircle(vv, x,y, 3,
		    VG_MapColorRGB(vv->vg->selectionColor));
	}
}

static void
Init(void *p)
{
	ES_LayoutSelectTool *t = p;

	t->flags = 0;
	t->vnMouseOver = NULL;
}

VG_ToolOps esLayoutSelectTool = {
	N_("Select PCB layout entities"),
	N_("Select / move PCB layout elements."),
	&esIconSelectArrow,
	sizeof(ES_LayoutSelectTool),
	VG_NOSNAP|VG_NOEDITCLEAR,
	Init,
	NULL,			/* destroy */
	NULL,			/* edit */
	NULL,			/* predraw */
	PostDraw,
	NULL,			/* selected */
	NULL,			/* deselected */
	MouseMotion,
	MouseButtonDown,
	MouseButtonUp,
	KeyDown,
	NULL			/* keyup */
};
