/*
 * Copyright (c) 2009 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Insert layout connection point.
 */

#include "core.h"
#include <agar/gui/primitive.h>

typedef struct es_layout_node_tool {
	VG_Tool _inherit;
} ES_LayoutNodeTool;

static int
MouseButtonDown(void *p, VG_Vector vPos, int button)
{
	ES_LayoutNodeTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Point *vp;
	ES_LayoutNode *sp;

	if (button != SDL_BUTTON_LEFT) {
		return (0);
	}
	if ((vp = VG_NearestPoint(vv, vPos, NULL)) != NULL) {
		sp = ES_LayoutNodeNew(vp);
	} else {
		sp = ES_LayoutNodeNew(vv->vg->root);
		VG_Translate(sp, vPos);
	}
	return (1);
}

static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int buttons)
{
	ES_LayoutNodeTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Point *vp;

	if ((vp = VG_HighlightNearestPoint(vv, vPos, NULL)) != NULL) {
		VG_Status(vv, _("Create connection point on Point%u"), VGNODE(vp)->handle);
	} else {
		VG_Status(vv, _("Create connection point at %f,%f"), vPos.x, vPos.y);
	}
	return (0);
}

static int
KeyDown(void *p, int ksym, int kmod, int unicode)
{
	ES_LayoutNodeTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Node *vn;
	Uint nDel = 0;

	if (ksym == SDLK_DELETE || ksym == SDLK_BACKSPACE) {
del:
		TAILQ_FOREACH(vn, &vv->vg->nodes, list) {
			if (!VG_NodeIsClass(vn, "LayoutNode") ||
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
		VG_Status(vv, _("Deleted %u connection points"), nDel);
		return (1);
	}
	return (0);
}

VG_ToolOps esLayoutNodeTool = {
	N_("Insert connection point"),
	N_("Create connection point in PCB layout."),
	&esIconPortEditor,
	sizeof(ES_LayoutNodeTool),
	VG_NOSNAP,
	NULL,
	NULL,			/* destroy */
	NULL,			/* edit */
	NULL,			/* predraw */
	NULL,			/* postdraw */
	NULL,			/* selected */
	NULL,			/* deselected */
	MouseMotion,
	MouseButtonDown,
	NULL,			/* mousebuttonup */
	KeyDown,
	NULL			/* keyup */
};
