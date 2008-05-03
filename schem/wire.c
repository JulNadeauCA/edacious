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

#include <eda.h>

static ES_Wire *esCurWire;

static void
Init(void *p)
{
	esCurWire = NULL;
}

static int
ConnectWire(ES_Circuit *ckt, ES_Wire *wire, VG_Vector p1, VG_Vector p2)
{
	ES_Port *port1, *port2;
	int N1, N2, N3;
	int i;

	ES_LockCircuit(ckt);

	port1 = VG_PointProximity(ckt->vg, "SchemPort", &p1, NULL, NULL);
	port2 = VG_PointProximity(ckt->vg, "SchemPort", &p2, NULL, NULL);
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
MouseButtonDown(void *p, VG_Vector vPos, int button)
{
	VG_Tool *t = p;
	char name[AG_OBJECT_NAME_MAX];
	VG_Vector v;
	ES_Circuit *ckt = t->p;
	ES_Component *com;
	ES_Wire *wire;
	ES_Port *port;
	int i;
	
	port = VG_PointProximity(ckt->vg, "SchemPort", &vPos, NULL, esCurWire);
	if (port != NULL) {
		if (port->com != NULL) {			/* Component */
			v = VG_Add(port->com->pos, port->pos);
		} else {					/* Wire */
			v = port->pos;
		}
	} else {
		v = vPos;
	}
	switch (button) {
	case SDL_BUTTON_LEFT:
		if (esCurWire == NULL) {
			wire = ES_CircuitAddWire(ckt);
			wire->ports[0].pos = v;
			wire->ports[1].pos = v;
			esCurWire = wire;
		} else {
			wire = esCurWire;
			wire->ports[0].flags &= ~(ES_PORT_SELECTED);
			wire->ports[1].flags &= ~(ES_PORT_SELECTED);
			if (ConnectWire(ckt, wire, wire->ports[0].pos,
			    wire->ports[1].pos) == -1) {
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
MouseButtonUp(void *p, VG_Vector vPos, int button)
{
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	ES_Port *port;

	switch (button) {
	case SDL_BUTTON_LEFT:
	case SDL_BUTTON_MIDDLE:
		ES_UnselectAllPorts(ckt);
		port = VG_PointProximity(ckt->vg, "SchemPort", &vPos, NULL,
		    esCurWire);
		if (port != NULL) {
			port->flags |= ES_PORT_SELECTED;
		}
		UpdateStatus(t->vgv, port);
		return (0);
	default:
		return (0);
	}
}

static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int buttons)
{
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	ES_Component *com;
	ES_Port *port;
	int i;

	ES_UnselectAllPorts(ckt);
	port = VG_PointProximity(ckt->vg, "SchemPort", &vPos, NULL, esCurWire);
	if (port != NULL) {
		port->flags |= ES_PORT_SELECTED;
	}
	UpdateStatus(t->vgv, port);
	if (esCurWire != NULL) {
		esCurWire->ports[1].pos = vPos;
	}
	return (0);
}

VG_ToolOps esWireTool = {
	N_("Wire"),
	N_("Merge two nodes."),
	&esIconInsertWire,
	sizeof(VG_Tool),
	0,
	Init,
	NULL,			/* destroy */
	NULL,			/* edit */
	NULL,			/* predraw */
	NULL,			/* postdraw */
	MouseMotion,
	MouseButtonDown,
	MouseButtonUp,
	NULL,			/* keydown */
	NULL			/* keyup */
};
