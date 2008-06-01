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

/* Return the Point nearest to vPos. */
void *
ES_SchemNearestPoint(VG_View *vv, VG_Vector vPos, void *ignore)
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
		v = vPos;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox < vv->grid[0].ival) {
			if (prox < proxNearest) {
				proxNearest = prox;
				vnNearest = vn;
			}
		}
	}
	return (vnNearest);
}

/* Highlight and return the Point nearest to vPos. */
void *
ES_SchemHighlightNearestPoint(VG_View *vv, VG_Vector vPos, void *ignore)
{
	VG *vg = vv->vg;
	float prox, proxNearest = AG_FLT_MAX;
	VG_Node *vn, *vnNearest = NULL;
	VG_Vector v;

	TAILQ_FOREACH(vn, &vg->nodes, list) {
		vn->flags &= ~(VG_NODE_MOUSEOVER);
		if (vn->ops->pointProximity == NULL ||
		    vn == ignore ||
		    !VG_NodeIsClass(vn, "Point")) {
			continue;
		}
		v = vPos;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox < vv->grid[0].ival) {
			if (prox < proxNearest) {
				proxNearest = prox;
				vnNearest = vn;
			}
		}
	}
	return (vnNearest);
}

/* Return the entity nearest to vPos. */
void *
ES_SchemNearest(VG_View *vv, VG_Vector vPos)
{
	VG *vg = vv->vg;
	float prox, proxNearest;
	VG_Node *vn, *vnNearest;
	VG_Vector v;

	/* First check if we intersect a block. */
	TAILQ_FOREACH(vn, &vg->nodes, list) {
		if (!VG_NodeIsClass(vn, "SchemBlock")) {
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
		if (prox <= PORT_RADIUS(vv)) {
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
	ES_SchemSelectTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Vector vSnap;
	VG_Node *vn;

	if (b != SDL_BUTTON_LEFT) {
		return (0);
	}
	if ((vn = ES_SchemNearest(vv, v)) != NULL) {
		if (SDL_GetModState() & KMOD_CTRL) {
			if (vn->flags & VG_NODE_SELECTED) {
				vn->flags &= ~(VG_NODE_SELECTED);
			} else {
				vn->flags |= VG_NODE_SELECTED;
			}
		} else {
			VG_UnselectAll(vv->vg);
			vn->flags |= VG_NODE_SELECTED;
		}
	}
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
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Node *vn;
	VG_Vector v;

	if (!t->moving) {
		TAILQ_FOREACH(vn, &vv->vg->nodes, list) {
			vn->flags &= ~(VG_NODE_MOUSEOVER);
		}
		if ((vn = ES_SchemNearest(vv, vPos)) != NULL) {
			vn->flags |= VG_NODE_MOUSEOVER;
		}
		return (0);
	}
	v = vPos;
	if (!VG_SKIP_CONSTRAINTS(vv)) {
		VG_ApplyConstraints(vv, &v);
	}
	TAILQ_FOREACH(vn, &vv->vg->nodes, list) {
		if (!(vn->flags & VG_NODE_SELECTED)) {
			continue;
		}
		if (vn->ops->moveNode != NULL)
			vn->ops->moveNode(vn, v, vRel);
	}
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
Init(void *p)
{
	ES_SchemSelectTool *t = p;

	t->moving = 0;
}

VG_ToolOps esSchemSelectTool = {
	N_("Select schematic entities"),
	N_("Select and move graphical elements."),
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
