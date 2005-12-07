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

const AG_Version component_ver = {
	"agar-eda component",
	0, 0
};

#define COMPONENT_REDRAW_DELAY	50

VG_ToolOps component_ins_tool;
VG_ToolOps component_sel_tool;

void
component_destroy(void *p)
{
	struct component *com = p;
	int i;

	pthread_mutex_destroy(&com->lock);
	for (i = 0; i < com->ndipoles; i++) {
		struct dipole *dip = &com->dipoles[i];

		Free(dip->loops, M_EDA);
		Free(dip->lpols, M_EDA);
	}
	Free(com->dipoles, M_EDA);
}

static void
select_component(struct component *com)
{
	com->selected = 1;
	AG_ObjMgrOpenData(com, 1);
}

static void
unselect_component(struct component *com)
{
	com->selected = 0;
	AG_ObjMgrCloseData(com);
}
		
static void
unselect_all_components(struct circuit *ckt)
{
	struct component *com;

	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!AGOBJECT_SUBCLASS(com, "component")) {
			continue;
		}
		unselect_component(com);
	}
}

static void
ComponentRenamed(AG_Event *event)
{
	struct component *com = AG_SELF();

	if (com->block == NULL)
		return;

	AG_ObjectCopyName(com, com->block->name, sizeof(com->block->name));
}

static void
ComponentAttached(AG_Event *event)
{
	char blkname[VG_BLOCK_NAME_MAX];
	struct component *com = AG_SELF();
	struct circuit *ckt = AG_SENDER();
	VG *vg = ckt->vg;
	VG_Block *block;

	if (!AGOBJECT_SUBCLASS(ckt, "circuit")) {
		return;
	}
	com->ckt = ckt;
	
	AG_ObjectCopyName(com, blkname, sizeof(blkname));
#ifdef DEBUG
	if (com->block != NULL) { Fatal("Block exists"); }
	if (VG_GetBlock(vg, blkname) != NULL) { Fatal("Duplicate block"); }
#endif
	com->block = VG_BeginBlock(vg, blkname, 0);
	VG_Origin(vg, 0, com->pin[0].x, com->pin[0].y);
	VG_EndBlock(vg);

	if (AGOBJECT_SUBCLASS(com, "component.vsource")) {
		ckt->vsrcs = Realloc(ckt->vsrcs,
		    (ckt->m+1)*sizeof(struct vsource *));
		ckt->vsrcs[ckt->m] = (struct vsource *)com;
		ckt->m++;
	}
}

static void
ComponentDetached(AG_Event *event)
{
	struct component *com = AG_SELF();
	struct circuit *ckt = AG_SENDER();
	u_int i, j;

	if (!AGOBJECT_SUBCLASS(ckt, "circuit"))
		return;
	
	if (AGOBJECT_SUBCLASS(com, "component.vsource")) {
		for (i = 0; i < ckt->m; i++) {
			if (ckt->vsrcs[i] == (struct vsource *)com) {
				if (i < --ckt->m) {
					for (; i < ckt->m; i++)
						ckt->vsrcs[i] = ckt->vsrcs[i+1];
				}
				break;
			}
		}
	}

	for (i = 0; i <= ckt->n; i++) {
		struct cktnode *node = ckt->nodes[i];
		struct cktbranch *br, *nbr;

		for (br = TAILQ_FIRST(&node->branches);
		     br != TAILQ_END(&node->branches);
		     br = nbr) {
			nbr = TAILQ_NEXT(br, branches);
			if (br->pin != NULL && br->pin->com == com) {
				circuit_del_branch(ckt, i, br);
			}
		}
	}

del_nodes:
	/* XXX widely inefficient */
	for (i = 1; i <= ckt->n; i++) {
		struct cktnode *node = ckt->nodes[i];
		struct cktbranch *br;

		if (node->nbranches == 0) {
			circuit_del_node(ckt, i);
			goto del_nodes;
		}
	}
	
	for (i = 1; i <= com->npins; i++) {
		struct pin *pin = &com->pin[i];

		pin->branch = NULL;
		pin->node = -1;
		pin->selected = 0;
	}

	for (i = 0; i < com->ndipoles; i++)
		com->dipoles[i].nloops = 0;

	if (com->block != NULL) {
		VG_DestroyBlock(ckt->vg, com->block);
		com->block = NULL;
	}
	com->ckt = NULL;
}

static void
ComponentShown(AG_Event *event)
{
	struct component *com = AG_SELF();

	AG_AddTimeout(com, &com->redraw_to, COMPONENT_REDRAW_DELAY);
}

static void
ComponentHidden(AG_Event *event)
{
	struct component *com = AG_SELF();

	AG_LockTimeouts(com);
	if (AG_TimeoutIsScheduled(com, &com->redraw_to)) {
		AG_DelTimeout(com, &com->redraw_to);
	}
	AG_UnlockTimeouts(com);
}

/* Redraw a component schematic block. */
static Uint32
ComponentRedraw(void *p, Uint32 ival, void *arg)
{
	struct component *com = p;
	VG_Block *block_save;
	VG_Element *element_save;
	VG *vg = com->ckt->vg;
	VG_Element *vge;
	int i;
	
	if ((AGOBJECT(com)->flags & AG_OBJECT_DATA_RESIDENT) == 0)
		return (ival);

#ifdef DEBUG
	if (com->block == NULL) { Fatal("Missing block"); }
#endif

	pthread_mutex_lock(&vg->lock);
	block_save = vg->cur_block;
	element_save = vg->cur_vge;
	VG_SelectBlock(vg, com->block);
	VG_ClearBlock(vg, com->block);

	if (com->ops->draw != NULL)
		com->ops->draw(com, vg);

	/* Indicate the nodes associated with connection points. */
	for (i = 1; i <= com->npins; i++) {
		struct pin *pin = &com->pin[i];
		double rho, theta;
		double x, y;

		VG_Car2Pol(vg, pin->x, pin->y, &rho, &theta);
		theta -= com->block->theta;
		VG_Pol2Car(vg, rho, theta, &x, &y);

		if (com->ckt->dis_nodes) {
			VG_Begin(vg, VG_CIRCLE);
			VG_Vertex2(vg, x, y);
			if (pin->selected) {
				VG_Color3(vg, 255, 255, 165);
				VG_CircleRadius(vg, 0.1600);
			} else {
				VG_Color3(vg, 0, 150, 150);
				VG_CircleRadius(vg, 0.0625);
			}
			VG_End(vg);
		}
		if (com->ckt->dis_node_names &&
		    pin->node >= 0) {
			VG_Begin(vg, VG_TEXT);
			VG_Vertex2(vg, x, y);
			VG_SetStyle(vg, "node-name");
			VG_Color3(vg, 0, 200, 100);
			VG_TextAlignment(vg, VG_ALIGN_BR);
			if (pin->node == 0) {
				VG_Printf(vg, "gnd");
			} else {
				VG_Printf(vg, "n%d", pin->node);
			}
			VG_End(vg);
		}
	}

	if (com->selected) {
		TAILQ_FOREACH(vge, &com->block->vges, vgbmbs) {
			VG_Select(vg, vge);
			/* TODO blend */
			VG_Color3(vg, 240, 240, 50);
			VG_End(vg);
		}
	} else if (com->highlighted) {
		TAILQ_FOREACH(vge, &com->block->vges, vgbmbs) {
			VG_Select(vg, vge);
			/* TODO blend */
			VG_Color3(vg, 180, 180, 120);
			VG_End(vg);
		}
	}

	if (com->block->theta != 0) {
		VG_RotateBlock(vg, com->block, com->block->theta);
	}
	VG_Select(vg, element_save);
	VG_SelectBlock(vg, block_save);
	pthread_mutex_unlock(&vg->lock);
	return (ival);
}

void
component_init(void *obj, const char *tname, const char *name, const void *ops,
    const struct pin *pinout)
{
	char cname[AG_OBJECT_NAME_MAX];
	struct component *com = obj;
	const struct pin *pinoutp;
	struct dipole *dip;
	int i, j, k;

	strlcpy(cname, "component.", sizeof(cname));
	strlcat(cname, tname, sizeof(cname));
	AG_ObjectInit(com, cname, name, ops);
	com->ops = ops;
	com->ckt = NULL;
	com->block = NULL;
	com->npins = 0;
	com->selected = 0;
	com->highlighted = 0;
	com->Tnom = 27+273.15;
	com->Tspec = 27+273.15;
	pthread_mutex_init(&com->lock, &agRecursiveMutexAttr);

	/* Assign the origin and default pinout. */
	com->pin[0].x = pinout[0].x;
	com->pin[0].y = pinout[0].y;
	for (i = 1, pinoutp = &pinout[1];
	     i < COMPONENT_MAX_PINS && pinoutp->n >= 0;
	     i++, pinoutp++) {
		struct pin *pin = &com->pin[i];

		strlcpy(pin->name, pinoutp->name, sizeof(pin->name));
		pin->n = i;
		pin->x = pinoutp->x;
		pin->y = pinoutp->y;
		pin->com = com;
		pin->node = -1;
		pin->branch = NULL;
		pin->u = 0;
		pin->du = 0;
		pin->selected = 0;
		com->npins++;
	}

	/* Find the dipoles. */
	com->dipoles = Malloc(sizeof(struct dipole), M_EDA);
	com->ndipoles = 0;
	for (i = 1; i <= com->npins; i++) {
		for (j = 1; j <= com->npins; j++) {
			if (i == j)
				continue;
			
			/* Skip redundant dipoles. */
			for (k = 0; k < com->ndipoles; k++) {
				struct dipole *dip = &com->dipoles[k];

				if (dip->p2 == &com->pin[i] &&
				    dip->p1 == &com->pin[j]) {
					break;
				}
			}
			if (k < com->ndipoles)
				continue;

			com->dipoles = Realloc(com->dipoles,
			    (com->ndipoles+1)*sizeof(struct dipole));
			dip = &com->dipoles[com->ndipoles++];
			dip->com = com;
			dip->p1 = &com->pin[i];
			dip->p2 = &com->pin[j];
			dip->loops = Malloc(sizeof(struct cktloop *), M_EDA);
			dip->lpols = Malloc(sizeof(int), M_EDA);
			dip->nloops = 0;
		}
	}

	AG_SetEvent(com, "attached", ComponentAttached, NULL);
	AG_SetEvent(com, "detached", ComponentDetached, NULL);
	AG_SetEvent(com, "renamed", ComponentRenamed, NULL);
	AG_SetEvent(com, "circuit-shown", ComponentShown, NULL);
	AG_SetEvent(com, "circuit-hidden", ComponentHidden, NULL);
	AG_SetTimeout(&com->redraw_to, ComponentRedraw, NULL,
	    AG_TIMEOUT_LOADABLE);
}

int
component_load(void *p, AG_Netbuf *buf)
{
	struct component *com = p;
	float Tnom, Tspec;

	if (AG_ReadVersion(buf, &component_ver, NULL) == -1)
		return (-1);

	com->flags = (u_int)AG_ReadUint32(buf);
	com->Tnom = AG_ReadFloat(buf);
	com->Tspec = AG_ReadFloat(buf);
	return (0);
}

int
component_save(void *p, AG_Netbuf *buf)
{
	struct component *com = p;

	AG_WriteVersion(buf, &component_ver);
	AG_WriteUint32(buf, com->flags);
	AG_WriteFloat(buf, com->Tnom);
	AG_WriteFloat(buf, com->Tspec);
	return (0);
}

/* Return the effective DC resistance of a dipole. */
double
dipole_resistance(struct dipole *dip)
{
#ifdef DEBUG
	if (dip->p1 == dip->p2) { Fatal("p1 == p2"); }
#endif
	if (dip->com->ops->resistance == NULL) {
		return (0);
	}
	if (dip->p1->n < dip->p2->n) {
		return (dip->com->ops->resistance(dip->com, dip->p1, dip->p2));
	} else {
		return (dip->com->ops->resistance(dip->com, dip->p2, dip->p1));
	}
}

/* Return the effective capacitance of a dipole. */
double
dipole_capacitance(struct dipole *dip)
{
#ifdef DEBUG
	if (dip->p1 == dip->p2)
		fatal("p1 == p2");
#endif
	if (dip->com->ops->capacitance == NULL) {
		return (0);
	}
	if (dip->p1->n < dip->p2->n) {
		return (dip->com->ops->capacitance(dip->com, dip->p1, dip->p2));
	} else {
		return (dip->com->ops->capacitance(dip->com, dip->p2, dip->p1));
	}
}

/* Return the effective inductance of a dipole. */
double
dipole_inductance(struct dipole *dip)
{
#ifdef DEBUG
	if (dip->p1 == dip->p2)
		fatal("p1 == p2");
#endif
	if (dip->com->ops->inductance == NULL) {
		return (0);
	}
	if (dip->p1->n < dip->p2->n) {
		return (dip->com->ops->inductance(dip->com, dip->p1, dip->p2));
	} else {
		return (dip->com->ops->inductance(dip->com, dip->p2, dip->p1));
	}
}

/*
 * Check whether a given dipole is part of a given loop, and return
 * its polarity with respect to the loop direction.
 */
int
dipole_in_loop(struct dipole *dip, struct cktloop *loop, int *sign)
{
	unsigned int i;

	for (i = 0; i < dip->nloops; i++) {
		if (dip->loops[i] == loop) {
			*sign = dip->lpols[i];
			return (1);
		}
	}
	return (0);
}

#ifdef EDITION
static struct component *sel_com = NULL;

void *
component_edit(void *p)
{
	struct component *com = p;
	AG_Window *win;

	if (com->ops->edit == NULL) {
		return (NULL);
	}
	win = com->ops->edit(com);
	AG_WindowSetPosition(win, AG_WINDOW_MIDDLE_LEFT, 0);
	AG_WindowSetCaption(win, "%s: %s", com->ops->name, AGOBJECT(com)->name);
	AG_WindowSetCloseAction(win, AG_WINDOW_DETACH);
	AG_WindowShow(win);
	return (win);
}

/* Insert a new, floating component into a circuit. */
void
component_insert(AG_Event *event)
{
	char name[AG_OBJECT_NAME_MAX];
	VG_View *vgv = AG_PTR(1);
	AG_Tlist *tl = AG_PTR(2);
	struct circuit *ckt = AG_PTR(3);
	AG_TlistItem *it;
	AG_ObjectType *comtype;
	struct component_ops *comops;
	struct component *com;
	VG_Tool *t;
	int n = 1;

	if ((it = AG_TlistSelectedItem(tl)) == NULL) {
		AG_TextMsg(AG_MSG_ERROR, _("No component type is selected."));
		return;
	}
	comtype = it->p1;
	comops = (struct component_ops *)comtype->ops;
tryname:
	snprintf(name, sizeof(name), "%s%d", comops->pfx, n++);
	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (strcmp(AGOBJECT(com)->name, name) == 0)
			break;
	}
	if (com != NULL)
		goto tryname;

	com = Malloc(comtype->size, M_OBJECT);
	comtype->ops->init(com, name);
	com->flags |= COMPONENT_FLOATING;
	AG_ObjectAttach(ckt, com);
	AG_ObjectUnlinkDatafiles(com);
	AG_ObjectPageIn(com, AG_OBJECT_DATA);
	AG_ObjectSave(com);
	AG_PostEvent(ckt, com, "circuit-shown", NULL);

	if ((t = VG_ViewFindTool(vgv, "Insert component")) != NULL) {
		VG_ViewSelectTool(vgv, t, ckt);
	}
	sel_com = com;
	sel_com->selected = 1;

	AG_WidgetFocus(vgv);
	AG_WindowFocus(AG_WidgetParentWindow(vgv));
}

/* Remove the selected components from the circuit. */
static int
remove_component(VG_Tool *t, SDLKey key, int state, void *arg)
{
	struct circuit *ckt = t->p;
	struct component *com;

	if (!state) {
		return (0);
	}
	AG_LockLinkage();
scan:
	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!AGOBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING ||
		    !com->selected) {
			continue;
		}
		if (AGOBJECT_SUBCLASS(com, "component.conductor")) {
			conductor_tool_reinit();
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
	circuit_modified(ckt);
	return (1);
}

static void
ins_tool_init(void *p)
{
	VG_Tool *t = p;

	sel_com = NULL;
	VG_ToolBindKey(t, KMOD_NONE, SDLK_DELETE, remove_component, NULL);
	VG_ToolBindKey(t, KMOD_NONE, SDLK_d, remove_component, NULL);
}

/*
 * Evaluate whether the given point collides with a pin (that does not
 * belong to the given component if specified).
 */
struct pin *
pin_overlap(struct circuit *ckt, struct component *ncom, double x,
    double y)
{
	struct component *ocom;
	int i;
	
	/* XXX use bounding boxes */
	AGOBJECT_FOREACH_CHILD(ocom, ckt, component) {
		if (!AGOBJECT_SUBCLASS(ocom, "component") ||
		    ocom == ncom ||
		    ocom->flags & COMPONENT_FLOATING) {
			continue;
		}
		for (i = 1; i <= ocom->npins; i++) {
			struct pin *opin = &ocom->pin[i];

			if (fabs(x-ocom->block->pos.x - opin->x) < 0.25 &&
			    fabs(y-ocom->block->pos.y - opin->y) < 0.25)
				return (opin);
		}
	}
	return (NULL);
}

/* Evaluate whether the given pin is connected to the reference point. */
int
pin_grounded(struct pin *pin)
{
	struct circuit *ckt = pin->com->ckt;

	return (cktnode_grounded(ckt, pin->node));
}

/* Connect a floating component to the circuit. */
void
component_connect(struct circuit *ckt, struct component *com, VG_Vtx *vtx)
{
	struct pin *opin;
	struct cktbranch *br;
	int i;

	for (i = 1; i <= com->npins; i++) {
		struct pin *pin = &com->pin[i];

		opin = pin_overlap(ckt, com, vtx->x+pin->x, vtx->y+pin->y);
		if (com->ops->connect == NULL ||
		    com->ops->connect(com, pin, opin) == -1) {
			if (opin != NULL) {
#ifdef DEBUG
				if (pin->node > 0)
					fatal("spurious node");
#endif
				pin->node = opin->node;
				pin->branch = circuit_add_branch(ckt,
				    opin->node, pin);
			} else {
				pin->node = circuit_add_node(ckt, 0);
				pin->branch = circuit_add_branch(ckt, pin->node,
				    pin);
			}
		}
		pin->selected = 0;
	}
	com->flags &= ~(COMPONENT_FLOATING);

	circuit_modified(ckt);
}

/*
 * Highlight the connection points of the given component with respect
 * to other components in the circuit and return the number of matches.
 */
int
component_highlight_pins(struct circuit *ckt, struct component *com)
{
	struct pin *opin;
	int i, nconn = 0;

	for (i = 1; i <= com->npins; i++) {
		struct pin *npin = &com->pin[i];

		opin = pin_overlap(ckt, com,
		    com->block->pos.x + npin->x,
		    com->block->pos.y + npin->y);
		if (opin != NULL) {
			npin->selected = 1;
			nconn++;
		} else {
			npin->selected = 0;
		}
	}
	return (nconn);
}

/* Rotate a floating component by 90 degrees. */
static void
rotate_component(struct circuit *ckt, struct component *com)
{
	VG *vg = ckt->vg;
	int i;

	com->block->theta += M_PI_2;

	for (i = 1; i <= com->npins; i++) {
		double rho, theta;

		VG_SelectBlock(vg, com->block);
		VG_Car2Pol(vg, com->pin[i].x, com->pin[i].y, &rho, &theta);
		theta += M_PI_2;
		VG_Pol2Car(vg, rho, theta, &com->pin[i].x, &com->pin[i].y);
		VG_EndBlock(vg);
	}
}

static int
ins_tool_buttondown(void *p, float x, float y, int b)
{
	VG_Tool *t = p;
	struct circuit *ckt = t->p;
	VG *vg = ckt->vg;
	VG_Vtx vtx;
	int i;
	
	switch (b) {
	case SDL_BUTTON_LEFT:
		if (sel_com != NULL) {
			vtx.x = x;
			vtx.y = y;
			component_connect(ckt, sel_com, &vtx);
			unselect_all_components(ckt);
			select_component(sel_com);
			sel_com = NULL;
			VG_ViewSelectTool(t->vgv, NULL, NULL);
		} else {
			printf("no component to connect\n");
		}
		break;
	case SDL_BUTTON_MIDDLE:
		if (sel_com != NULL) {
			rotate_component(ckt, sel_com);
		} else {
			printf("no component to rotate\n");
		}
		break;
	default:
		return (0);
	}
	return (1);
remove:
	if (sel_com != NULL) {
		AG_ObjectDetach(sel_com);
		AG_ObjectUnlinkDatafiles(sel_com);
		AG_ObjectDestroy(sel_com);
		Free(sel_com, M_OBJECT);
		sel_com = NULL;
		VG_ViewSelectTool(t->vgv, NULL, NULL);
	}
	return (1);
}

/* Select overlapping components. */
static int
sel_tool_mousebuttondown(void *p, float x, float y, int b)
{
	VG_Tool *t = p;
	struct circuit *ckt = t->p;
	VG *vg = ckt->vg;
	struct component *com;
	int multi = SDL_GetModState() & KMOD_CTRL;
	AG_Window *pwin;

	if (b != SDL_BUTTON_LEFT)
		return (0);

	if (!multi) {
		unselect_all_components(ckt);
	}
	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		VG_Rect rext;

		if (!AGOBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		VG_BlockExtent(vg, com->block, &rext);
		if (x > rext.x && y > rext.y &&
		    x < rext.x+rext.w && y < rext.y+rext.h) {
			if (multi) {
				if (com->selected) {
					unselect_component(com);
				} else {
					select_component(com);
				}
			} else {
				select_component(com);
				break;
			}
		}
	}
	if ((pwin = AG_WidgetParentWindow(t->vgv)) != NULL) {
		agView->focus_win = pwin;
		AG_WidgetFocus(t->vgv);
	}
	return (1);
}

/* Select overlapping components. */
static int
sel_tool_leftbutton(VG_Tool *t, int button, int state, float x, float y,
    void *arg)
{
	struct circuit *ckt = t->p;
	VG *vg = ckt->vg;
	struct component *com;
	int multi = SDL_GetModState() & KMOD_CTRL;

	if (button != SDL_BUTTON_LEFT || !state)
		return (0);

	if (!multi) {
		unselect_all_components(ckt);
	}
	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		VG_Rect rext;

		if (!AGOBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		VG_BlockExtent(vg, com->block, &rext);
		if (x > rext.x && y > rext.y &&
		    x < rext.x+rext.w && y < rext.y+rext.h) {
			if (multi) {
				if (com->selected) {
					unselect_component(com);
				} else {
					select_component(com);
				}
			} else {
				select_component(com);
				break;
			}
		}
	}
	return (1);
}

static void
sel_tool_init(void *p)
{
	VG_Tool *t = p;

	VG_ToolBindMouseButton(t, SDL_BUTTON_LEFT, sel_tool_leftbutton, NULL);
}

/* Move a floating component and highlight the overlapping nodes. */
static int
ins_tool_mousemotion(void *p, float x, float y, float xrel, float yrel, int b)
{
	VG_Tool *t = p;
	struct circuit *ckt = t->p;
	VG *vg = ckt->vg;
	int nconn;

	vg->origin[1].x = x;
	vg->origin[1].y = y;
	if (sel_com != NULL) {
		VG_MoveBlock(vg, sel_com->block, x, y, -1);
		nconn = component_highlight_pins(ckt, sel_com);
		vg->redraw++;
		return (1);
	}
	return (0);
}

/* Highlight any component underneath the cursor. */
static int
sel_tool_mousemotion(void *p, float x, float y, float xrel, float yrel, int b)
{
	VG_Tool *t = p;
	struct circuit *ckt = t->p;
	VG *vg = ckt->vg;
	struct component *com;
	VG_Rect rext;

	vg->origin[1].x = x;
	vg->origin[1].y = y;
	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!AGOBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		VG_BlockExtent(vg, com->block, &rext);
		if (x > rext.x &&
		    y > rext.y &&
		    x < rext.x+rext.w &&
		    y < rext.y+rext.h) {
			com->highlighted = 1;
		} else {
			com->highlighted = 0;
		}
	}
	return (0);
}

VG_ToolOps component_ins_tool = {
	N_("Insert component"),
	N_("Insert new components into the circuit."),
	EDA_INSERT_COMPONENT_ICON,
	sizeof(VG_Tool),
	0,
	ins_tool_init,
	NULL,			/* destroy */
	NULL,			/* edit */
	ins_tool_mousemotion,
	ins_tool_buttondown,
	NULL,			/* mousebuttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};

VG_ToolOps component_sel_tool = {
	N_("Select component"),
	N_("Select one or more component in the circuit."),
	EDA_COMPONENT_ICON,
	sizeof(VG_Tool),
	0,
	sel_tool_init,
	NULL,			/* destroy */
	NULL,			/* edit */
	sel_tool_mousemotion,
	sel_tool_mousebuttondown,
	NULL,			/* mousebuttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};
#endif /* EDITION */

#ifdef EDITION
void
component_reg_menu(AG_Menu *m, AG_MenuItem *pitem, struct circuit *ckt)
{
	/* Nothing yet */
}
#endif /* EDITION */
