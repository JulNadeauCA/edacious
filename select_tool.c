/*
 * Copyright (c) 2004-2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * "Select component" tool.
 */

#include <eda.h>

static int
MouseButtonDown(void *p, VG_Vector vPos, int button)
{
#if 0
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	ES_Component *com;
	AG_Window *pwin;
	VG_Block *blkClosest;
	int multi = SDL_GetModState() & KMOD_CTRL;

	if (b != SDL_BUTTON_LEFT) {
		return (0);
	}
	blkClosest = VG_BlockClosest(ckt->vg, x, y);

	if (!multi) {
		ES_ComponentUnselectAll(ckt);
	}
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->block != blkClosest) {
			continue;
		}
		if (multi) {
			if (com->selected) {
				ES_ComponentUnselect(com);
			} else {
				ES_ComponentSelect(com);
			}
		} else {
			ES_ComponentSelect(com);
			break;
		}
	}
	if (t->vgv != NULL &&
	    (pwin = AG_WidgetParentWindow(t->vgv)) != NULL) {
		AG_WidgetFocus(t->vgv);
		AG_WindowFocus(pwin);
	}
#endif
	return (1);
}

#if 0
static int
LeftButton(VG_Tool *t, int button, int state, float x, float y, void *arg)
{
	ES_Circuit *ckt = t->p;
	ES_Component *com;
	VG_Block *blkClosest;
	int multi = SDL_GetModState() & KMOD_CTRL;

	if (button != SDL_BUTTON_LEFT || !state) {
		return (0);
	}
	blkClosest = VG_BlockClosest(ckt->vg, x, y);

	if (!multi) {
		ES_ComponentUnselectAll(ckt);
	}
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->block != blkClosest) {
			continue;
		}
		if (multi) {
			if (com->selected) {
				ES_ComponentUnselect(com);
			} else {
				ES_ComponentSelect(com);
			}
		} else {
			ES_ComponentSelect(com);
			break;
		}
	}
	return (1);
}
#endif

static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int buttons)
{
#if 0
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	VG *vg = ckt->vg;
	ES_Component *com;
	VG_Rect rext;
	VG_Block *blkClosest;
	
	blkClosest = VG_BlockClosest(vg, x, y);
	
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		com->highlighted = (com->block == blkClosest);
	}
#endif
	return (0);
}

static void
Init(void *p)
{
	VG_Tool *t = p;
#if 0
	VG_ToolBindMouseButton(t, SDL_BUTTON_LEFT, LeftButton, NULL);
#endif
}

VG_ToolOps esSelectTool = {
	N_("Select circuit objects"),
	N_("Select components and connections in the circuit."),
	&esIconSelectComponent,
	sizeof(VG_Tool),
	VG_NOSNAP,
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