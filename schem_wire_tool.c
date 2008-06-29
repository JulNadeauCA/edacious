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
 * "Wire" tool. This tool insert a wire in the schematic and modifies the
 * circuit topology (i.e., merging circuit nodes or creating new ones).
 */

#include <eda.h>
#include "wire.h"

static ES_Wire *esCurWire;

static void
Init(void *p)
{
	esCurWire = NULL;
}

/*
 * Merge and create the nodes and branches needed to connect the two specified
 * points in the schematic.
 */
static int
ConnectWire(VG_View *vv, ES_Circuit *ckt, ES_Wire *wire, VG_Vector p1,
    VG_Vector p2)
{
	ES_SchemPort *sp1, *sp2;
	int N1, N2, N3;
	int i;

	ES_LockCircuit(ckt);

	sp1 = VG_PointProximity(vv, "SchemPort", &p1, NULL, NULL);
	sp2 = VG_PointProximity(vv, "SchemPort", &p2, NULL, NULL);
	if ((sp1 != NULL && sp1->port->node != -1) &&
	    (sp2 != NULL && sp2->port->node != -1) &&
	    (sp1->port->node == sp2->port->node)) {
		AG_SetError(_("Redundant connection"));
		goto fail;
	}

	N1 = (sp1->port != NULL && sp1->port->node != -1) ?
	    sp1->port->node : ES_AddNode(ckt);
	N2 = (sp2->port != NULL && sp2->port->node != -1) ?
	    sp2->port->node : ES_AddNode(ckt);
	COMPONENT(wire)->ports[1].node = N1;
	COMPONENT(wire)->ports[2].node = N2;

	if ((N3 = ES_MergeNodes(ckt, N1, N2)) == -1) {
		goto fail;
	}
	sp1->port->node = N3;
	sp2->port->node = N3;
	COMPONENT(wire)->ports[1].node = N3;
	COMPONENT(wire)->ports[2].node = N3;
	ES_AddBranch(ckt, N3, &COMPONENT(wire)->ports[1]);
	ES_AddBranch(ckt, N3, &COMPONENT(wire)->ports[2]);
	COMPONENT(wire)->flags &= ~(ES_COMPONENT_FLOATING);

	VG_Status(vv, _("Connected %s:%d and %s:%d as n%d"),
	    OBJECT(sp1->port->com)->name, sp1->port->n,
	    OBJECT(sp2->port->com)->name, sp2->port->n, N3);

	ES_CircuitModified(ckt);
	ES_UnlockCircuit(ckt);
	return (0);
fail:
	ES_UnlockCircuit(ckt);
	return (-1);
}

static void
UpdateStatus(VG_View *vv, ES_SchemPort *sp)
{
	if (sp == NULL || sp->port == NULL) {
		VG_Status(vv, _("Create new node"));
		return;
	}
	if (sp->port->com != NULL) {
		VG_Status(vv, _("Connect to %s:%s (n%d)"),
		    OBJECT(sp->port->com)->name, sp->port->name, 
		    sp->port->node);
	} else {
		VG_Status(vv, _("Connect to n%d"), sp->port->node);
	}
}

static int
MouseButtonDown(void *p, VG_Vector vPos, int button)
{
	VG_Tool *t = p;
	VG_View *vv = t->vgv;
	VG *vg = vv->vg;
	ES_Circuit *ckt = t->p;
	ES_Component *com;
	ES_Port *port1, *port2;
	ES_SchemPort *sp;
	ES_SchemWire *sw;
	int i;
	
	switch (button) {
	case SDL_BUTTON_LEFT:
		if (esCurWire == NULL) {
			/* Create the Circuit Wire object */
			esCurWire = ES_WireNew(ckt);
			port1 = &COMPONENT(esCurWire)->ports[1];
			port2 = &COMPONENT(esCurWire)->ports[2];

			/* Create the schematic Port entities. */
			port1->sp = ES_SchemPortNew(vg->root);
			port2->sp = ES_SchemPortNew(vg->root);
			port1->sp->com = COMPONENT(esCurWire);
			port2->sp->com = COMPONENT(esCurWire);
			port1->sp->port = port1;
			port2->sp->port = port2;
			VG_Translate(port1->sp, vPos);
			VG_Translate(port2->sp, vPos);

			/* Create the schematic Wire entity. */
			sw = ES_SchemWireNew(vg->root, port1->sp, port2->sp);
			sw->wire = esCurWire;
			esCurWire->schemWire = sw;
			ES_AttachSchemEntity(esCurWire, VGNODE(sw));
		} else {
			port1 = &COMPONENT(esCurWire)->ports[1];
			port2 = &COMPONENT(esCurWire)->ports[2];
			ES_UnselectPort(port1);
			ES_UnselectPort(port2);
			if (ConnectWire(vv, ckt, esCurWire, VG_Pos(port1->sp),
			    VG_Pos(port2->sp)) == -1) {
				AG_TextMsg(AG_MSG_ERROR, "%s", AG_GetError());
				VG_Status(vv, _("Could not connect: %s"),
				    AG_GetError());
			} else {
				esCurWire = NULL;
			}
		}
		ES_UnselectAllPorts(ckt);
		return (1);
	case SDL_BUTTON_RIGHT:
		if (esCurWire != NULL) {
			AG_ObjectDetach(esCurWire);
			AG_ObjectDestroy(esCurWire);
			esCurWire = NULL;
			ES_UnselectAllPorts(ckt);
			return (1);
		}
		return (0);
	default:
		return (0);
	}
}

static int
MouseButtonUp(void *p, VG_Vector vPos, int button)
{
	VG_Tool *t = p;
	VG_View *vv = t->vgv;
	ES_Circuit *ckt = t->p;
	ES_SchemPort *spNear;

	switch (button) {
	case SDL_BUTTON_LEFT:
		ES_UnselectAllPorts(ckt);
		spNear = VG_PointProximityMax(vv, "SchemPort", &vPos, NULL,
		    esCurWire, PORT_RADIUS(vv));
		if (spNear != NULL) {
			ES_SelectPort(spNear->port);
		}
		UpdateStatus(vv, spNear);
		return (0);
	default:
		return (0);
	}
}

static void
PostDraw(void *p, VG_View *vv)
{
	VG_Tool *t = p;
	int x, y;

	VG_GetViewCoords(vv, VGTOOL(t)->vCursor, &x,&y);
	AG_DrawCircle(vv, x,y, 3, VG_MapColorRGB(vv->vg->selectionColor));
}

static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int buttons)
{
	VG_Tool *t = p;
	VG_View *vv = t->vgv;
	ES_Circuit *ckt = t->p;
	ES_Component *com;
	ES_SchemPort *spNear;
	int i;

	ES_UnselectAllPorts(ckt);
	spNear = VG_PointProximityMax(vv, "SchemPort", &vPos, NULL,
	    (esCurWire != NULL) ? COMPONENT(esCurWire)->ports[1].sp : NULL,
	    PORT_RADIUS(vv));
	if (spNear != NULL) {
		ES_SelectPort(spNear->port);
	}
	if (esCurWire != NULL) {
		VG_SetPosition(COMPONENT(esCurWire)->ports[1].sp,
		    (spNear != NULL) ? VG_Pos(spNear) : vPos);
	}
	UpdateStatus(vv, spNear);
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
	PostDraw,
	MouseMotion,
	MouseButtonDown,
	MouseButtonUp,
	NULL,			/* keydown */
	NULL			/* keyup */
};
