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
 * Generate plots of proximity functions for debugging purposes.
 */

#include <eda.h>
#include <agar/core/limits.h>

#ifdef DEBUG

static int
MouseButtonDown(void *t, VG_Vector v, int button)
{
	VG_View *vv = VGTOOL(t)->vgv;
	VG_Node *vn;

	if (button == SDL_BUTTON_LEFT) {
		vn = ES_SchemNearest(vv, v);
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
		return (1);
	}
	return (0);
}

static void
PostDraw(void *t, VG_View *vv)
{
	const int rSize = 5;
	AG_Rect r;
	VG_Color c;
	VG_Node *vn;
	VG_Vector v;
	float prox;
	float vx, vy;
	float vRange = 80.0f;

	r.w = rSize;
	r.h = rSize;
	for (r.y = 0; r.y < AGWIDGET(vv)->h; r.y+=r.h-1) {
		for (r.x = 0; r.x < AGWIDGET(vv)->w; r.x+=r.w-1) {
			prox = AG_FLT_MAX;
			TAILQ_FOREACH(vn, &vv->vg->nodes, list) {
				if ((vn->flags & VG_NODE_SELECTED) == 0 ||
				    vn->ops->pointProximity == NULL) {
					continue;
				}
				VG_GetVGCoordsFlt(vv,
				    VGVECTOR(r.x+(r.w/2), r.y+(r.h/2)), &v);
				prox = vn->ops->pointProximity(vn, vv, &v);
				break;
			}
			if (prox < vRange) {
				c.r = 0;
				c.g = 255 - (Uint8)(prox*255.0f/vRange);
				c.b = 0;
				AG_DrawRectFilled(vv, r, VG_MapColorRGB(c));
			}
		}
	}
}

VG_ToolOps esSchemProximityTool = {
	N_("Proximity"),
	N_("Plot proximity functions of selected entities."),
	&esIconProximity,
	sizeof(VG_Tool),
	VG_NOSNAP,
	NULL,			/* init */
	NULL,			/* destroy */
	NULL,			/* edit */
	NULL,			/* predraw */
	PostDraw,
	NULL,			/* mousemotion */
	MouseButtonDown,
	NULL,			/* buttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};
#endif /* DEBUG */