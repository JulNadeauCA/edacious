/*
 * Copyright (c) 2006-2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * "Wire" tool. This tool either merges nodes or creates new ones.
 */

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>

#include "eda.h"

static ES_Wire *esCurWire;

static void
InitTool(void *p)
{
	esCurWire = NULL;
}

static int
ConnectWire(ES_Circuit *ckt, ES_Wire *wire, float x1, float y1, float x2,
    float y2)
{
	ES_Port *port1, *port2;
	int N1, N2, N3;
	int i;

	ES_LockCircuit(ckt);

	port1 = ES_PortNearestPoint(ckt, x1,y1, NULL);
	port2 = ES_PortNearestPoint(ckt, x2,y2, NULL);
	if ((port1 != NULL && port1->node != -1) &&
	    (port2 != NULL && port2->node != -1) &&
	    (port1->node == port2->node)) {
		AG_SetError(_("Redundant connection"));
		goto fail;
	}
	N1 = (port1 != NULL && port1->node != -1) ? port1->node :
	    ES_CircuitAddNode(ckt, 0);
	N2 = (port2 != NULL && port2->node != -1) ? port2->node :
	    ES_CircuitAddNode(ckt, 0);
	wire->ports[0].node = N1;
	wire->ports[1].node = N2;

	if ((N3 = ES_CircuitMergeNodes(ckt, N1, N2)) == -1) {
		goto fail;
	}
	port1->node = N3;
	port2->node = N3;
	wire->ports[0].node = N3;
	wire->ports[1].node = N3;
	ES_CircuitAddBranch(ckt, N3, &wire->ports[0]);
	ES_CircuitAddBranch(ckt, N3, &wire->ports[1]);

	ES_CircuitModified(ckt);
	ES_UnlockCircuit(ckt);
	return (0);
fail:
	ES_UnlockCircuit(ckt);
	return (-1);
}

static void
UpdateStatus(VG_View *vv, ES_Port *port)
{
	if (port == NULL) {
		VG_Status(vv, _("Create new node"));
		return;
	}
	if (port->com != NULL) {
		VG_Status(vv, _("Connect to %s:%s (n%d)"),
		    OBJECT(port->com)->name, port->name, port->node);
	} else {
		VG_Status(vv, _("Connect to n%d"), port->node);
	}
}

static int
MouseButtonDown(void *p, float xCurs, float yCurs, int b)
{
	VG_Tool *t = p;
	char name[AG_OBJECT_NAME_MAX];
	float x, y;
	ES_Circuit *ckt = t->p;
	ES_Component *com;
	ES_Wire *wire;
	ES_Port *port;
	int i;
	
	if ((port = ES_PortNearestPoint(ckt, xCurs, yCurs, esCurWire))
	    != NULL) {
		if (port->com != NULL) {			/* Component */
			x = port->com->block->pos.x+port->x;
			y = port->com->block->pos.y+port->y;
		} else {					/* Wire */
			x = port->x;
			y = port->y;
		}
	} else {
		x = xCurs;
		y = yCurs;
	}
	switch (b) {
	case SDL_BUTTON_LEFT:
		if (esCurWire == NULL) {
			wire = ES_CircuitAddWire(ckt);
			wire->ports[0].x = x;
			wire->ports[0].y = y;
			wire->ports[1].x = x;
			wire->ports[1].y = y;
			esCurWire = wire;
		} else {
			wire = esCurWire;
			wire->ports[0].flags &= ~(ES_PORT_SELECTED);
			wire->ports[1].flags &= ~(ES_PORT_SELECTED);
			if (ConnectWire(ckt, wire,
			    wire->ports[0].x, wire->ports[0].y,
			    wire->ports[1].x, wire->ports[1].y) == -1) {
				AG_TextMsg(AG_MSG_ERROR, "%s", AG_GetError());
				VG_Status(t->vgv, _("Could not connect: %s"),
				    AG_GetError());
			} else {
				esCurWire = NULL;
			}
		}
		ES_UnselectAllPorts(ckt);
		return (1);
	default:
		if (esCurWire != NULL) {
			ES_CircuitDelWire(ckt, esCurWire);
			esCurWire = NULL;
		}
		ES_UnselectAllPorts(ckt);
		return (0);
	}
}

static int
MouseButtonUp(void *p, float x, float y, int b)
{
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	ES_Port *port;

	switch (b) {
	case SDL_BUTTON_LEFT:
	case SDL_BUTTON_MIDDLE:
		ES_UnselectAllPorts(ckt);
		if ((port = ES_PortNearestPoint(ckt, x,y, esCurWire)) != NULL) {
			port->flags |= ES_PORT_SELECTED;
		}
		UpdateStatus(t->vgv, port);
		return (0);
	default:
		return (0);
	}
}

static int
MouseMotion(void *p, float x, float y, float xrel, float yrel, int b)
{
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	ES_Component *com;
	ES_Port *port;
	int i;

	ES_UnselectAllPorts(ckt);
	if ((port = ES_PortNearestPoint(ckt, x, y, esCurWire)) != NULL) {
		port->flags |= ES_PORT_SELECTED;
	}
	UpdateStatus(t->vgv, port);
	if (esCurWire != NULL) {
		esCurWire->ports[1].x = x;
		esCurWire->ports[1].y = y;
	}
	return (0);
}

VG_ToolOps esWireTool = {
	N_("Wire"),
	N_("Merge two nodes."),
	&esIconInsertWire,
	sizeof(VG_Tool),
	0,
	InitTool,
	NULL,				/* destroy */
	NULL,				/* edit */
	MouseMotion,
	MouseButtonDown,
	MouseButtonUp,
	NULL,				/* keydown */
	NULL				/* keyup */
};
