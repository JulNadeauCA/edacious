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
 * "Insert component" tool.
 */

#include <eda.h>

ES_Component *esInscomCur = NULL;

/* Remove the selected components from the circuit. */
static int
RemoveComponent(VG_Tool *t, SDLKey key, int state, void *arg)
{
	ES_Circuit *ckt = t->p;
	ES_Component *com;

	if (!state) {
		return (0);
	}
	AG_LockVFS(ckt);
	ES_LockCircuit(ckt);
scan:
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->flags & COMPONENT_FLOATING ||
		    !com->selected) {
			continue;
		}
		ES_CloseEditionWindow(com);
		if (AG_ObjectInUse(com)) {
			AG_TextMsg(AG_MSG_ERROR, "%s: %s", OBJECT(com)->name,
			    AG_GetError());
			continue;
		}
		AG_ObjectDetach(com);
		AG_ObjectUnlinkDatafiles(com);
		AG_ObjectDestroy(com);
		goto scan;
	}
	AG_UnlockVFS(ckt);
	ES_CircuitModified(ckt);
	ES_UnlockCircuit(ckt);
	return (1);
}

#if 0
/* Rotate a floating component by 90 degrees. */
static void
RotateComponent(ES_Circuit *ckt, ES_Component *com)
{
	VG *vg = ckt->vg;
	ES_Port *port;
	float r, theta;
	int i;

	COMPONENT_FOREACH_PORT(port, i, com) {
		r = VG_Hypot(port->pos.x, port->pos.y);
		theta = VG_Atan2(port->pos.y, port->pos.x) + M_PI_2;
		port->pos.x = r*VG_Cos(theta);
		port->pos.y = r*VG_Sin(theta);
	}
}
#endif

/* Highlight the component connections that would be made to existing nodes. */
static void
HighlightConnections(ES_Circuit *ckt, ES_Component *com)
{
	ES_Port *port, *nearestPort;
	VG_Vector pos;
	int i;

	COMPONENT_FOREACH_PORT(port, i, com) {
		if (port->schemPort == NULL) {
			fprintf(stderr, "Port%d has no schem entity!\n", i);
			continue;
		}
		pos = VG_PointPos(port->schemPort->p);
		nearestPort = VG_PointProximityMax(ckt->vg, "SchemPort", &pos,
		    NULL, com, ckt->vg->gridIval);
		if (nearestPort != NULL) {
			port->flags |= ES_PORT_SELECTED;
		} else {
			port->flags &= ~(ES_PORT_SELECTED);
		}
	}
}

/* Connect a floating component to the circuit. */
static void
ConnectComponent(ES_Circuit *ckt, ES_Component *com, VG_Vector pos)
{
	VG_Vector portPos;
	ES_Port *port, *nearestPort;
	ES_Branch *br;
	int i;

	ES_LockCircuit(ckt);
	Debug(ckt, "Connecting component: %s\n", OBJECT(com)->name);
	Debug(com, "Connecting to circuit: %s\n", OBJECT(ckt)->name);
	com->pos = pos;
	COMPONENT_FOREACH_PORT(port, i, com) {
		portPos = VG_PointPos(port->schemPort->p);
		nearestPort = VG_PointProximityMax(ckt->vg, "SchemPort",
		    &portPos, NULL, com, ckt->vg->gridIval);
		if (COMOPS(com)->connect == NULL ||
		    COMOPS(com)->connect(com, port, nearestPort) == -1) {
			if (nearestPort != NULL) {
				port->node = nearestPort->node;
				port->branch = ES_CircuitAddBranch(ckt,
				    nearestPort->node, port);
			} else {
				port->node = ES_CircuitAddNode(ckt, 0);
				port->branch = ES_CircuitAddBranch(ckt,
				    port->node, port);
			}
		}
		port->flags &= ~(ES_PORT_SELECTED);
	}
	com->flags &= ~(COMPONENT_FLOATING);

	AG_PostEvent(ckt, com, "circuit-connected", NULL);
	ES_CircuitModified(ckt);
	ES_UnlockCircuit(ckt);
}


static int
MouseButtonDown(void *p, VG_Vector vPos, int button)
{
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	VG *vg = ckt->vg;
	int i;
	
	switch (button) {
	case SDL_BUTTON_LEFT:
		if (esInscomCur != NULL) {
			ConnectComponent(ckt, esInscomCur, vPos);
			ES_ComponentUnselectAll(ckt);
			ES_ComponentSelect(esInscomCur);
			esInscomCur = NULL;
			VG_ViewSelectTool(t->vgv, NULL, NULL);
		}
		break;
#if 0
	case SDL_BUTTON_MIDDLE:
		if (esInscomCur != NULL)
			RotateComponent(ckt, esInscomCur);
		break;
#endif
	default:
		return (0);
	}
	return (1);
remove:
	if (esInscomCur != NULL) {
		ES_LockCircuit(ckt);
		AG_ObjectDetach(esInscomCur);
		AG_ObjectUnlinkDatafiles(esInscomCur);
		AG_ObjectDestroy(esInscomCur);
		esInscomCur = NULL;
		VG_ViewSelectTool(t->vgv, NULL, NULL);
		ES_UnlockCircuit(ckt);
	}
	return (1);
}

/* Move a floating component and highlight the overlapping nodes. */
static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int b)
{
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	VG *vg = ckt->vg;

	if (esInscomCur != NULL) {
		esInscomCur->pos = vPos;
		HighlightConnections(ckt, esInscomCur);
		return (1);
	}
	return (0);
}

static void
Init(void *p)
{
	VG_Tool *t = p;

	esInscomCur = NULL;
	VG_ToolBindKey(t, KMOD_NONE, SDLK_DELETE, RemoveComponent, NULL);
	VG_ToolBindKey(t, KMOD_NONE, SDLK_d, RemoveComponent, NULL);
}

VG_ToolOps esInscomOps = {
	N_("Insert component"),
	N_("Insert new components into the circuit."),
	NULL,
	sizeof(VG_Tool),
	0,
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
