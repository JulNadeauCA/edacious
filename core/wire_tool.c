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

#include "core.h"

typedef struct es_wire_tool {
	VG_Tool _inherit;
	Uint flags;
#define CREATE_NEW_NODES 0x01	/* Allow creation of new nodes */
	ES_Wire *curWire;	/* Wire being created */
} ES_WireTool;

static void
Init(void *p)
{
	ES_WireTool *t = p;

	t->flags = CREATE_NEW_NODES;
	t->curWire = NULL;
}
	
/* Create a new floating wire instance. */
static void
CreateWire(ES_WireTool *t, VG_Vector vPos)
{
	VG_View *vv = VGTOOL(t)->vgv;
	ES_Circuit *ckt = VGTOOL(t)->p;
	ES_Port *port1, *port2;
	ES_SchemWire *sw;
	ES_SchemPort *spNear;

	ES_LockCircuit(ckt);
	
	/* Create the Circuit Wire object. */
	t->curWire = ES_WireNew(ckt);
	port1 = &COMPONENT(t->curWire)->ports[1];
	port2 = &COMPONENT(t->curWire)->ports[2];

	/* Insert the schematic entities. */
	spNear = VG_PointProximityMax(vv, "SchemPort", &vPos, NULL,
	    t->curWire, vv->pointSelRadius);

	if (spNear != NULL) {
		port1->node = spNear->port->node;
	} else {
		if ((t->flags & CREATE_NEW_NODES) == 0) {
			VG_Status(vv, _("Node creation disabled"));
			goto out;
		}
		port1->node = ES_AddNode(ckt);
	}
	VG_Status(vv, _("Connecting to n%d..."), port1->node);

	port1->sp = ES_SchemPortNew(ckt->vg->root, port1);
	port2->sp = ES_SchemPortNew(ckt->vg->root, port2);
	VG_Translate(port1->sp, vPos);
	VG_Translate(port2->sp, vPos);

	sw = ES_SchemWireNew(ckt->vg->root, port1->sp, port2->sp);
	sw->wire = t->curWire;
	ES_AttachSchemEntity(t->curWire, VGNODE(sw));
out:
	ES_UnlockCircuit(ckt);
}

/*
 * Merge and create the nodes and branches needed to connect the two specified
 * points in the schematic.
 */
static void
ConnectWire(ES_WireTool *t, VG_Vector vPos)
{
	ES_Port *port1 = &COMPONENT(t->curWire)->ports[1];
	ES_Port *port2 = &COMPONENT(t->curWire)->ports[2];
	VG_Vector pos2 = VG_Pos(port2->sp);
	ES_Wire *wire = t->curWire;
	VG_View *vv = VGTOOL(t)->vgv;
	ES_Circuit *ckt = VGTOOL(t)->p;
	ES_SchemPort *spNear;
	int nMerged;

	ES_LockCircuit(ckt);
	ES_UnselectPort(port1);
	ES_UnselectPort(port2);

	spNear = VG_PointProximityMax(vv, "SchemPort", &pos2, NULL,
	    port2->sp, vv->pointSelRadius);
	if (spNear != NULL && spNear->port != NULL &&
	    spNear->port->node != -1 &&
	    port1->node != spNear->port->node) {
		port2->node = spNear->port->node;
		VG_NodeDetach(port2->sp);
		VG_LoadIdentity(port2->sp);
		VG_NodeAttach(spNear, port2->sp);
	} else {
		if ((t->flags & CREATE_NEW_NODES) == 0) {
			AG_SetError(_("Node creation disabled"));
			goto fail;
		}
		port2->node = ES_AddNode(ckt);
	}

	/* Merge the two nodes, add branches to the Wire component. */
	if ((nMerged = ES_MergeNodes(ckt, port1->node, port2->node)) == -1) {
		goto fail;
	}
	port1->node = nMerged;
	port2->node = nMerged;
	ES_AddBranch(ckt, nMerged, port1);
	ES_AddBranch(ckt, nMerged, port2);

	/* Connect the Wire component to the Circuit. */
	TAILQ_INSERT_TAIL(&ckt->components, COMPONENT(wire), components);
	COMPONENT(wire)->flags |= ES_COMPONENT_CONNECTED;

	VG_Status(vv, _("Connected as n%d"), nMerged);

	ES_CircuitModified(ckt);
	ES_UnlockCircuit(ckt);
	t->curWire = NULL;
	return;
fail:
	ES_UnlockCircuit(ckt);
	VG_StatusS(vv, AG_GetError());
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
	ES_WireTool *t = p;
	ES_Circuit *ckt = VGTOOL(t)->p;
	
	switch (button) {
	case AG_MOUSE_LEFT:
		if (t->curWire == NULL) {
			CreateWire(t, vPos);
		} else {
			ConnectWire(t, vPos);
		}
		ES_UnselectAllPorts(ckt);
		return (1);
	case AG_MOUSE_RIGHT:
		if (t->curWire != NULL) {
			AG_ObjectDetach(t->curWire);
			AG_ObjectDestroy(t->curWire);
			t->curWire = NULL;
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
	ES_WireTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	ES_Circuit *ckt = VGTOOL(t)->p;
	ES_SchemPort *spNear;

	switch (button) {
	case AG_MOUSE_LEFT:
		ES_UnselectAllPorts(ckt);
		spNear = VG_PointProximityMax(vv, "SchemPort", &vPos, NULL,
		    t->curWire, vv->pointSelRadius);
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
	AG_Color c = VG_MapColorRGB(vv->vg->selectionColor);
	int x, y;

	VG_GetViewCoords(vv, t->vCursor, &x,&y);
	AG_DrawCircle(vv, x,y, 3, &c);
}

static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int buttons)
{
	ES_WireTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	ES_Circuit *ckt = VGTOOL(t)->p;
	ES_SchemPort *spCur = (t->curWire != NULL) ?
	                      COMPONENT(t->curWire)->ports[2].sp : NULL;
	ES_SchemPort *spNear;

	ES_UnselectAllPorts(ckt);
	if ((spNear = VG_PointProximityMax(vv, "SchemPort", &vPos, NULL,
	    spCur, vv->pointSelRadius)) != NULL) {
		ES_SelectPort(spNear->port);
	}
	if (spCur != NULL) {
		VG_SetPosition(spCur, (spNear != NULL) ? VG_Pos(spNear) : vPos);
	}
	UpdateStatus(vv, spNear);
	return (0);
}

static void *
Edit(void *p, VG_View *vv)
{
	ES_WireTool *t = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	AG_CheckboxNewFlag(box, 0, _("Create new nodes"),
	    &t->flags, CREATE_NEW_NODES);
	return (box);
}

VG_ToolOps esWireTool = {
	N_("Wire"),
	N_("Merge two nodes."),
	&esIconInsertWire,
	sizeof(ES_WireTool),
	0,
	Init,
	NULL,			/* destroy */
	Edit,
	NULL,			/* predraw */
	PostDraw,
	NULL,			/* selected */
	NULL,			/* deselected */
	MouseMotion,
	MouseButtonDown,
	MouseButtonUp,
	NULL,			/* keydown */
	NULL			/* keyup */
};
