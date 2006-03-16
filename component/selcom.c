/*	$Csoft: component.c,v 1.7 2005/09/27 03:34:08 vedge Exp $	*/

/*
 * Copyright (c) 2004, 2005, 2006 Hypertriton, Inc.
 * <http://www.hypertriton.com/>
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

#include <agar/core.h>
#include <agar/vg.h>
#include <agar/gui.h>

#include "eda.h"

/* Select overlapping components. */
static int
ES_SelcomButtondown(void *p, float x, float y, int b)
{
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	ES_Component *com;
	AG_Window *pwin;
	VG_Block *closest_blk;
	int multi = SDL_GetModState() & KMOD_CTRL;

	if (b != SDL_BUTTON_LEFT) {
		return (0);
	}
	closest_blk = VG_BlockClosest(ckt->vg, x, y);

	if (!multi) {
		ES_ComponentUnselectAll(ckt);
	}
	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "component") {
		if (com->flags & COMPONENT_FLOATING ||
		    com->block != closest_blk) {
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
		agView->focus_win = pwin;
		AG_WidgetFocus(t->vgv);
	}
	return (1);
}

#if 0
/* Select overlapping components. */
static int
ES_SelcomLeftButton(VG_Tool *t, int button, int state, float x, float y,
    void *arg)
{
	ES_Circuit *ckt = t->p;
	ES_Component *com;
	VG_Block *closest_blk;
	int multi = SDL_GetModState() & KMOD_CTRL;

	if (button != SDL_BUTTON_LEFT || !state) {
		return (0);
	}
	closest_blk = VG_BlockClosest(ckt->vg, x, y);

	if (!multi) {
		ES_ComponentUnselectAll(ckt);
	}
	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "component") {
		if (com->flags & COMPONENT_FLOATING ||
		    com->block != closest_blk) {
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

/* Highlight any component underneath the cursor. */
static int
ES_SelcomMousemotion(void *p, float x, float y, float xrel, float yrel, int b)
{
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	VG *vg = ckt->vg;
	ES_Component *com;
	VG_Rect rext;
	VG_Block *closest_blk;
	
	vg->origin[1].x = x;
	vg->origin[1].y = y;
	closest_blk = VG_BlockClosest(vg, x, y);
	
	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "component") {
		if (com->flags & COMPONENT_FLOATING) {
			continue;
		}
		com->highlighted = (com->block == closest_blk);
	}
	return (0);
}

static void
ES_SelcomInit(void *p)
{
	VG_Tool *t = p;
#if 0
	VG_ToolBindMouseButton(t, SDL_BUTTON_LEFT, ES_SelcomLeftButton, NULL);
#endif
}

VG_ToolOps esSelcomOps = {
	N_("Select component"),
	N_("Select one or more component in the circuit."),
	EDA_COMPONENT_ICON,
	sizeof(VG_Tool),
	VG_NOSNAP,
	ES_SelcomInit,
	NULL,			/* destroy */
	NULL,			/* edit */
	ES_SelcomMousemotion,
	ES_SelcomButtondown,
	NULL,			/* mousebuttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};
