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

#include "core.h"
#include <agar/core/limits.h>

typedef struct es_schem_select_tool {
	VG_Tool _inherit;
	Uint flags;
#define MOVING_ENTITIES	0x01	/* Translation is in progress */
	VG_Vector vLast;	/* For grid snapping */
	VG_Node *vnMouseOver;	/* Element under cursor */
} ES_SchemSelectTool;

static int
MouseButtonDown(void *p, VG_Vector v, int b)
{
	ES_SchemSelectTool *t = p;
	ES_SchemBlock *sb;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Node *vn;

	if ((vn = ES_SchemNearest(vv, v)) == NULL) {
		return (0);
	}
	switch (b) {
	case SDL_BUTTON_LEFT:
		t->vLast = v;
		if (VG_SELECT_MULTI(vv)) {
			if (vn->flags & VG_NODE_SELECTED) {
				vn->flags &= ~(VG_NODE_SELECTED);
			} else {
				vn->flags |= VG_NODE_SELECTED;

				if (VG_NodeIsClass(vn, "SchemBlock")) {
					sb = (ES_SchemBlock *)vn;
					ES_SelectComponent(sb->com, vv);
				} else {
					VG_EditNode(vv, 0, vn);
				}
			}
		} else {
			if (VG_NodeIsClass(vn, "SchemBlock")) {
				sb = (ES_SchemBlock *)vn;
				ES_UnselectAllComponents(COMCIRCUIT(sb->com),
				    vv);
				ES_SelectComponent(sb->com, vv);
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
	ES_SchemSelectTool *t = p;

	if (b != SDL_BUTTON_LEFT) {
		return (0);
	}
	t->flags &= ~(MOVING_ENTITIES);
	return (1);
}

static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int buttons)
{
	ES_SchemSelectTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Node *vn;
	VG_Vector v;

	/* Provide visual feedback of current selection. */
	if ((t->flags & MOVING_ENTITIES) == 0) {
		if ((vn = ES_SchemNearest(vv, vPos)) != NULL &&
		    t->vnMouseOver != vn) {
			t->vnMouseOver = vn;
			if (VG_NodeIsClass(vn, "SchemBlock")) {
				ES_HighlightComponent(SCHEM_BLOCK(vn)->com);
				VG_Status(vv, _("Select component: %s"),
				    OBJECT(SCHEM_BLOCK(vn)->com)->name);
			} else if (VG_NodeIsClass(vn, "SchemWire")) {
				VG_Status(vv, _("Select wire (n%d)"),
				    COMPONENT(SCHEM_WIRE(vn)->wire)->ports[1].node);
			} else if (VG_NodeIsClass(vn, "SchemPort")) {
				VG_Status(vv, _("Select port (n%d)"),
				    SCHEM_PORT(vn)->port->node);
			} else {
				VG_Status(vv, _("Select schematic entity: %s%d"),
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
				VG_ClearEditAreas(vv);
				if (VG_NodeIsClass(vn, "SchemWire")) {
					AG_ObjectDetach(SCHEM_WIRE(vn)->wire);
				} else if (VG_NodeIsClass(vn, "SchemBlock")) {
					AG_ObjectDetach(SCHEM_BLOCK(vn)->com);
				} else {
					if (VG_Delete(vn) == -1) {
						vn->flags &= ~(VG_NODE_SELECTED);
						VG_Status(vv, "%s%u: %s",
						    vn->ops->name, vn->handle,
						    AG_GetError());
						return (0);
					}
				}
				nDel++;
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

	t->flags = 0;
	t->vnMouseOver = NULL;
}

VG_ToolOps esSchemSelectTool = {
	N_("Select schematic entities"),
	N_("Select / move circuit schematic elements."),
	&esIconSelectArrow,
	sizeof(ES_SchemSelectTool),
	VG_NOSNAP|VG_NOEDITCLEAR,
	Init,
	NULL,			/* destroy */
	NULL,			/* edit */
	NULL,			/* predraw */
	NULL,			/* postdraw */
	NULL,			/* selected */
	NULL,			/* deselected */
	MouseMotion,
	MouseButtonDown,
	MouseButtonUp,
	KeyDown,
	NULL			/* keyup */
};
