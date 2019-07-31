/*
 * Copyright (c) 2004-2009 Hypertriton, Inc. <http://hypertriton.com/>
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

#include "core.h"

typedef struct es_component_insert_tool {
	VG_Tool _inherit;
	Uint flags;
#define INSERT_MULTIPLE	0x01		/* Insert multiple instances */
	ES_Component *floatingCom;
} ES_ComponentInsertTool;

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
		    port->sp, vv->pointSelRadius);
		if (spNear != NULL) {
			ES_SelectPort(port);
			Snprintf(s, sizeof(s), "%d>[%s:%d] ", i,
			    OBJECT(spNear->port->com)->name, spNear->port->n);
		} else {
			ES_UnselectPort(port);
			Snprintf(s, sizeof(s), _("%d->(new) "), i);
		}
		Strlcat(status, s, sizeof(status));
	}
	VG_StatusS(vv, status);
}

/*
 * Connect a floating component to the circuit. When new SchemPort entities
 * overlap existing ones, create a new Branch in the existing node. Where
 * there is no overlap, create new nodes.
 */
static int
ConnectComponent(ES_ComponentInsertTool *t, ES_Circuit *ckt, ES_Component *com)
{
	VG_View *vv = VGTOOL(t)->vgv;
	VG *vg = ckt->vg;
	VG_Vector portPos;
	ES_Port *port, *portNear;
	ES_SchemPort *spNear;
	int i;

	ES_LockCircuit(ckt);

	Debug(ckt, "Connecting component: %s\n", OBJECT(com)->name);
	Debug(com, "Connecting to circuit: %s\n", OBJECT(ckt)->name);

	/*
	 * Connect the component to the circuit. Add branches corresponding to
	 * the overlapping SchemPorts. It is possible to override this
	 * behavior in connect()
	 */
	COMPONENT_FOREACH_PORT(port, i, com) {
		if (port->sp == NULL) {
			continue;
		}
		portPos = VG_Pos(port->sp);
		spNear = VG_PointProximityMax(vv, "SchemPort", &portPos, NULL,
		    port->sp, vv->pointSelRadius);
		portNear = (spNear != NULL) ? spNear->port : NULL;

		if (COMCLASS(com)->connect == NULL ||
		    COMCLASS(com)->connect(com, port, portNear) == -1) {
			if (portNear != NULL) {
				if (portNear->node == -1) {
					AG_SetError(_("Cannot connect "
					              "component to itself!"));
					return (-1);
				}
				port->node = portNear->node;
				port->branch = ES_AddBranch(ckt, portNear->node,
				    port);
			} else {
				port->node = ES_AddNode(ckt);
				port->branch = ES_AddBranch(ckt, port->node,
				    port);
			}
		}

		/* Create implicit Wires between the ports. */
		if (spNear != NULL) {
			ES_Wire *w;
			ES_SchemWire *sw;
			ES_Port *P1, *P2;

			w = ES_WireNew(ckt);
			P1 = &COMPONENT(w)->ports[1];
			P2 = &COMPONENT(w)->ports[2];
			P1->node = port->node;
			P2->node = port->node;
			P1->sp = ES_SchemPortNew(vg->root, P1);
			P2->sp = ES_SchemPortNew(vg->root, P2);
			sw = ES_SchemWireNew(vg->root, P1->sp, P2->sp);
			sw->wire = w;
			ES_AttachSchemEntity(w, VGNODE(sw));
			ES_AddBranch(ckt, port->node, P1);
			ES_AddBranch(ckt, port->node, P2);
			VG_Translate(P1->sp, portPos);
			VG_Translate(P2->sp, portPos);

			TAILQ_INSERT_TAIL(&ckt->components, COMPONENT(w),
			    components);
			COMPONENT(w)->flags |= ES_COMPONENT_CONNECTED;
		}

		ES_UnselectPort(port);
	}

	/* Connect to the circuit. The component is no longer floating. */
	TAILQ_INSERT_TAIL(&ckt->components, com, components);
	com->flags |= ES_COMPONENT_CONNECTED;
	AG_PostEvent(ckt, com, "circuit-connected", NULL);
	ES_CircuitModified(ckt);

	ES_UnlockCircuit(ckt);
	
	ES_UnselectAllComponents(ckt, vv);
	ES_SelectComponent(t->floatingCom, vv);
	t->floatingCom = NULL;
#if 0
	/* Insert more instances... */
	if (ES_InsertComponent(ckt, VGTOOL(t), model) == -1) {
		AG_TextError("Inserting additional instance: %s", AG_GetError());
	}
#endif
	return (0);
}

/* Remove the current floating component if any. */
static void
RemoveFloatingComponent(ES_ComponentInsertTool *t)
{
	if (t->floatingCom != NULL) {
		AG_ObjectDetach(t->floatingCom);
		AG_ObjectDestroy(t->floatingCom);
		t->floatingCom = NULL;
	}
}

static int
MouseButtonDown(void *p, VG_Vector vPos, int button)
{
	ES_ComponentInsertTool *t = p;
	ES_Circuit *ckt = VGTOOL(t)->p;
	VG_View *vv = VGTOOL(t)->vgv;
	
	switch (button) {
	case AG_MOUSE_LEFT:
		if (t->floatingCom != NULL) {
			if (ConnectComponent(t, ckt, t->floatingCom) == -1){
				AG_TextError("Connecting component: %s",
				    AG_GetError());
				break;
			}
			if (t->floatingCom != NULL) {
				VG_Node *vn;

				TAILQ_FOREACH(vn, &t->floatingCom->schemEnts,
				    user) {
					VG_SetPosition(vn, vPos);
				}
				HighlightConnections(vv, ckt, t->floatingCom);
				return (1);
			}
		}
		break;
	case AG_MOUSE_MIDDLE:
		if (t->floatingCom != NULL) {
			AG_KeyMod mod = AG_GetModState(vv);
			VG_Node *vn;

			TAILQ_FOREACH(vn, &t->floatingCom->schemEnts, user) {
				if (mod & AG_KEYMOD_CTRL) {
					VG_Rotate(vn, VG_PI/4.0f);
				} else {
					VG_Rotate(vn, VG_PI/2.0f);
				}
			}
		}
		break;
	case AG_MOUSE_RIGHT:
		RemoveFloatingComponent(t);
		VG_ViewSelectTool(vv, NULL, ckt);
		return (1);
	default:
		return (0);
	}
	return (1);
}

/* Move a floating component and highlight the overlapping nodes. */
static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int b)
{
	ES_ComponentInsertTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	ES_Circuit *ckt = VGTOOL(t)->p;
	VG_Node *vn;

	if (t->floatingCom != NULL) {
		TAILQ_FOREACH(vn, &t->floatingCom->schemEnts, user) {
			VG_SetPosition(vn, vPos);
		}
		HighlightConnections(vv, ckt, t->floatingCom);
		return (1);
	}
	return (0);
}

/* Insert a new floating component instance in the circuit. */
static void
Insert(AG_Event *event)
{
	char name[AG_OBJECT_NAME_MAX];
	ES_ComponentInsertTool *t = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	ES_Component *comModel = AG_PTR(3), *com;
	VG_View *vv = VGTOOL(t)->vgv;

	ES_LockCircuit(ckt);

	/* Allocate the instance and initialize from the model file. */
	if ((com = malloc(COMCLASS(comModel)->obj.size)) == NULL) {
		AG_SetError("Out of memory");
		goto fail;
	}
	AG_ObjectInit(com, COMCLASS(comModel));

	if (!AG_Defined(comModel,"archive-path")) {
		goto fail;
	}
	if (AG_ObjectLoadFromFile(com,
	    AG_GetStringP(comModel,"archive-path")) == -1) {
		AG_ObjectDestroy(com);
		goto fail;
	}

	/* Generate a unique component name */
	AG_ObjectGenNamePfx(ckt, COMCLASS(comModel)->pfx, name, sizeof(name));
	AG_ObjectSetNameS(com, name);

	/* Attach to the circuit as a floating component. */
	t->floatingCom = com;
	AG_ObjectAttach(ckt, com);
	ES_SelectComponent(com, vv);
	AG_PostEvent(ckt, com, "circuit-shown", NULL);

	AG_WidgetFocus(vv);
	ES_UnlockCircuit(ckt);
	return;
fail:
	AG_TextMsgFromError();
	ES_UnlockCircuit(ckt);
}

/* Abort the current insert operation. */
static void
AbortInsert(AG_Event *event)
{
	ES_ComponentInsertTool *t = AG_PTR(1);

	RemoveFloatingComponent(t);
	VG_ViewSelectTool(VGTOOL(t)->vgv, NULL, NULL);
}

static void
Init(void *p)
{
	ES_ComponentInsertTool *t = p;
	VG_ToolCommand *tc;

	t->floatingCom = NULL;
	t->flags = INSERT_MULTIPLE;

	VG_ToolCommandNew(t, "Insert", Insert);
	tc = VG_ToolCommandNew(t, "AbortInsert", AbortInsert);
	VG_ToolCommandKey(tc, AG_KEYMOD_NONE, AG_KEY_ESCAPE);
}

static void *
Edit(void *p, VG_View *vv)
{
	ES_ComponentInsertTool *t = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	AG_CheckboxNewFlag(box, 0, _("Insert multiple instances"),
	    &t->flags, INSERT_MULTIPLE);
	return (box);
}

VG_ToolOps esComponentInsertTool = {
	N_("Insert component"),
	N_("Insert new components into the circuit."),
	NULL,
	sizeof(ES_ComponentInsertTool),
	0,
	Init,
	NULL,			/* destroy */
	Edit,
	NULL,			/* predraw */
	NULL,			/* postdraw */
	NULL,			/* selected */
	NULL,			/* deselected */
	MouseMotion,
	MouseButtonDown,
	NULL,			/* mousebuttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};
