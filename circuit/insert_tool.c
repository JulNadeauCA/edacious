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

ES_Component *esFloatingCom = NULL;

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
		if (COMPONENT_IS_FLOATING(com) || !com->selected) {
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

/* Highlight the component connections that would be made to existing nodes. */
static void
HighlightConnections(VG_View *vv, ES_Circuit *ckt, ES_Component *com)
{
	char status[4096], s[128];
	ES_Port *port;
	ES_SchemPort *spNear;
	VG_Vector pos;
	int i;

	status[0] = '\0';
	COMPONENT_FOREACH_PORT(port, i, com) {
		if (port->sp == NULL) {
			continue;
		}
		pos = VG_Pos(port->sp);
		spNear = VG_PointProximityMax(vv, "SchemPort", &pos, NULL,
		    port->sp, PORT_RADIUS(vv));
		if (spNear != NULL) {
			port->flags |= ES_PORT_SELECTED;
			snprintf(s, sizeof(s), "%d>[%s:%d] ",
			    i, OBJECT(spNear->com)->name, spNear->port->n);
		} else {
			port->flags &= ~(ES_PORT_SELECTED);
			snprintf(s, sizeof(s), _("%d->(new) "), i);
		}
		Strlcat(status, s, sizeof(status));
	}
	VG_Status(vv, "%s", status);
}

/*
 * Connect a floating component to the circuit. Create branches for overlapping
 * ports, otherwise create new nodes.
 */
static int
ConnectComponent(VG_View *vv, ES_Circuit *ckt, ES_Component *com)
{
	VG_Vector portPos;
	ES_Port *port, *portNear;
	ES_SchemPort *spNear;
	ES_Branch *br;
	int i;

	ES_LockCircuit(ckt);
	Debug(ckt, "Connecting component: %s\n", OBJECT(com)->name);
	Debug(com, "Connecting to circuit: %s\n", OBJECT(ckt)->name);
	COMPONENT_FOREACH_PORT(port, i, com) {
		fprintf(stderr, "Connecting %s:%i\n", OBJECT(com)->name, i);
		if (port->sp == NULL) {
			continue;
		}
		portPos = VG_Pos(port->sp);
		spNear = VG_PointProximityMax(vv, "SchemPort", &portPos, NULL,
		    port->sp, PORT_RADIUS(vv));
		portNear = (spNear != NULL) ? spNear->port : NULL;

		if (COMOPS(com)->connect == NULL ||
		    COMOPS(com)->connect(com, port, portNear) == -1) {
			if (portNear != NULL) {
				if (portNear->node == -1) {
					AG_SetError(_("Cannot connect "
					              "component to itself!"));
					return (-1);
				}
				port->node = portNear->node;
				port->branch = ES_CircuitAddBranch(ckt,
				    portNear->node, port);
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
	return (0);
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
		if (esFloatingCom != NULL) {
			if (ConnectComponent(t->vgv, ckt, esFloatingCom) == -1){
				AG_TextMsgFromError();
				break;
			}
			ES_ComponentUnselectAll(ckt);
			ES_ComponentSelect(esFloatingCom);
			esFloatingCom = NULL;
			VG_ViewSelectTool(t->vgv, NULL, NULL);
		}
		break;
	case SDL_BUTTON_MIDDLE:
		if (esFloatingCom != NULL) {
			ES_SchemBlock *blk;

			TAILQ_FOREACH(blk, &esFloatingCom->blocks, blocks)
				VG_Rotate(blk, VG_PI/2.0f);
		}
		break;
	case SDL_BUTTON_RIGHT:
		if (esFloatingCom != NULL) {
			ES_LockCircuit(ckt);
			AG_ObjectDetach(esFloatingCom);
			AG_ObjectUnlinkDatafiles(esFloatingCom);
#if 0
			AG_ObjectDestroy(esFloatingCom);
#endif
			esFloatingCom = NULL;
			VG_ViewSelectTool(t->vgv, NULL, NULL);
			ES_UnlockCircuit(ckt);
			return (1);
		}
		break;
	default:
		return (0);
	}
	return (1);
}

/* Move a floating component and highlight the overlapping nodes. */
static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int b)
{
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	ES_SchemBlock *sb;

	if (esFloatingCom != NULL) {
		TAILQ_FOREACH(sb, &esFloatingCom->blocks, blocks) {
			VG_SetPosition(sb, vPos);
		}
		HighlightConnections(t->vgv, ckt, esFloatingCom);
		return (1);
	}
	return (0);
}

static void
Init(void *p)
{
	VG_Tool *t = p;

	esFloatingCom = NULL;
	VG_ToolBindKey(t, KMOD_NONE, SDLK_DELETE, RemoveComponent, NULL);
	VG_ToolBindKey(t, KMOD_NONE, SDLK_d, RemoveComponent, NULL);
}

VG_ToolOps esInsertTool = {
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
