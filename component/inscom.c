/*	$Csoft: component.c,v 1.7 2005/09/27 03:34:08 vedge Exp $	*/

/*
 * Copyright (c) 2004, 2005 CubeSoft Communications, Inc.
 * <http://www.winds-triton.com>
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

ES_Component *esInscomCur = NULL;

/* Remove the selected components from the circuit. */
static int
remove_component(VG_Tool *t, SDLKey key, int state, void *arg)
{
	ES_Circuit *ckt = t->p;
	ES_Component *com;

	if (!state) {
		return (0);
	}
	AG_LockLinkage();
scan:
	AGOBJECT_FOREACH_CHILD(com, ckt, es_component) {
		if (!AGOBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING ||
		    !com->selected) {
			continue;
		}
		if (AGOBJECT_SUBCLASS(com, "component.conductor")) {
			ES_ConductorToolReinit();
		}
		AG_ObjMgrCloseData(com);
		if (AG_ObjectInUse(com)) {
			AG_TextMsg(AG_MSG_ERROR, "%s: %s", AGOBJECT(com)->name,
			    AG_GetError());
			continue;
		}
		AG_ObjectDetach(com);
		AG_ObjectUnlinkDatafiles(com);
		AG_ObjectDestroy(com);
		Free(com, M_OBJECT);
		goto scan;
	}
	AG_UnlockLinkage();
	ES_CircuitModified(ckt);
	return (1);
}

/* Rotate a floating component by 90 degrees. */
static void
rotate_component(ES_Circuit *ckt, ES_Component *com)
{
	VG *vg = ckt->vg;
	int i;

	com->block->theta += M_PI_2;

	for (i = 1; i <= com->nports; i++) {
		float rho, theta;

		VG_SelectBlock(vg, com->block);
		VG_Car2Pol(vg, com->ports[i].x, com->ports[i].y, &rho, &theta);
		theta += M_PI_2;
		VG_Pol2Car(vg, rho, theta, &com->ports[i].x, &com->ports[i].y);
		VG_EndBlock(vg);
	}
}

static int
InscomButtondown(void *p, float x, float y, int b)
{
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	VG *vg = ckt->vg;
	VG_Vtx vtx;
	int i;
	
	switch (b) {
	case SDL_BUTTON_LEFT:
		if (esInscomCur != NULL) {
			vtx.x = x;
			vtx.y = y;
			ES_ComponentConnect(ckt, esInscomCur, &vtx);
			ES_ComponentUnselectAll(ckt);
			ES_ComponentSelect(esInscomCur);
			esInscomCur = NULL;
			VG_ViewSelectTool(t->vgv, NULL, NULL);
		} else {
			printf("no component to connect\n");
		}
		break;
	case SDL_BUTTON_MIDDLE:
		if (esInscomCur != NULL) {
			rotate_component(ckt, esInscomCur);
		} else {
			printf("no component to rotate\n");
		}
		break;
	default:
		return (0);
	}
	return (1);
remove:
	if (esInscomCur != NULL) {
		AG_ObjectDetach(esInscomCur);
		AG_ObjectUnlinkDatafiles(esInscomCur);
		AG_ObjectDestroy(esInscomCur);
		Free(esInscomCur, M_OBJECT);
		esInscomCur = NULL;
		VG_ViewSelectTool(t->vgv, NULL, NULL);
	}
	return (1);
}

/* Move a floating component and highlight the overlapping nodes. */
static int
InscomMousemotion(void *p, float x, float y, float xrel, float yrel, int b)
{
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	VG *vg = ckt->vg;
	int nconn;

	vg->origin[1].x = x;
	vg->origin[1].y = y;
	if (esInscomCur != NULL) {
		VG_MoveBlock(vg, esInscomCur->block, x, y, -1);
		nconn = ES_ComponentHighlightPorts(ckt, esInscomCur);
		vg->redraw++;
		return (1);
	}
	return (0);
}

static void
InscomInit(void *p)
{
	VG_Tool *t = p;

	esInscomCur = NULL;
	VG_ToolBindKey(t, KMOD_NONE, SDLK_DELETE, remove_component, NULL);
	VG_ToolBindKey(t, KMOD_NONE, SDLK_d, remove_component, NULL);
}

VG_ToolOps esInscomOps = {
	N_("Insert component"),
	N_("Insert new components into the circuit."),
	EDA_INSERT_COMPONENT_ICON,
	sizeof(VG_Tool),
	0,
	InscomInit,
	NULL,			/* destroy */
	NULL,			/* edit */
	InscomMousemotion,
	InscomButtondown,
	NULL,			/* mousebuttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};