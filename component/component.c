/*	$Csoft: component.c,v 1.6 2005/09/15 02:04:49 vedge Exp $	*/

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

#include <engine/engine.h>
#include <engine/widget/window.h>
#include <engine/view.h>
#include <engine/typesw.h>
#include <engine/objmgr.h>

#include <engine/map/map.h>

#include <engine/vg/vg.h>
#include <engine/vg/vg_math.h>

#ifdef EDITION
#include <engine/widget/tlist.h>
#include <engine/widget/label.h>
#include <engine/widget/fspinbutton.h>
#include <engine/widget/units.h>
#include <engine/widget/menu.h>

#include <engine/map/mapview.h>
#include <engine/map/tool.h>
#endif

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "aeda.h"

#include <circuit/circuit.h>
#include <component/vsource.h>
#include <component/conductor.h>

const AG_Version component_ver = {
	"agar-eda component",
	0, 0
};

#define COMPONENT_REDRAW_DELAY	50

AG_MaptoolOps component_ins_tool;
AG_MaptoolOps component_sel_tool;

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
renamed(int argc, union evarg *argv)
{
	struct component *com = argv[0].p;

	if (com->block == NULL)
		return;

	AG_ObjectCopyName(com, com->block->name, sizeof(com->block->name));
}

static void
attached(int argc, union evarg *argv)
{
	char blkname[VG_BLOCK_NAME_MAX];
	struct component *com = argv[0].p;
	struct circuit *ckt = argv[argc].p;
	VG *vg = ckt->vg;
	VG_Block *block;

	if (!AGOBJECT_SUBCLASS(ckt, "circuit")) {
		return;
	}
	com->ckt = ckt;
	
	AG_ObjectCopyName(com, blkname, sizeof(blkname));
#ifdef DEBUG
	if (com->block != NULL)
		fatal("block exists");
	if (VG_GetBlock(vg, blkname) != NULL)
		fatal("duplicate block: %s", blkname);
#endif
	com->block = VG_BeginBlock(vg, blkname, 0);
	VG_Origin2(vg, 0, com->pin[0].x, com->pin[0].y);
	VG_EndBlock(vg);

	if (AGOBJECT_SUBCLASS(com, "component.vsource")) {
		ckt->vsrcs = Realloc(ckt->vsrcs,
		    (ckt->m+1)*sizeof(struct vsource *));
		ckt->vsrcs[ckt->m] = (struct vsource *)com;
		ckt->m++;
	}
}

static void
detached(int argc, union evarg *argv)
{
	struct component *com = argv[0].p;
	struct circuit *ckt = argv[argc].p;
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
shown(int argc, union evarg *argv)
{
	struct component *com = argv[0].p;

	AG_AddTimeout(com, &com->redraw_to, COMPONENT_REDRAW_DELAY);
}

static void
hidden(int argc, union evarg *argv)
{
	struct component *com = argv[0].p;

	AG_LockTimeouts(com);
	if (AG_TimeoutIsScheduled(com, &com->redraw_to)) {
		AG_DelTimeout(com, &com->redraw_to);
	}
	AG_UnlockTimeouts(com);
}

/* Redraw a component schematic block. */
static Uint32
redraw(void *p, Uint32 ival, void *arg)
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
	if (com->block == NULL)
		fatal("%s: no block", AGOBJECT(com)->name);
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

	AG_SetEvent(com, "attached", attached, NULL);
	AG_SetEvent(com, "detached", detached, NULL);
	AG_SetEvent(com, "renamed", renamed, NULL);
	AG_SetEvent(com, "circuit-shown", shown, NULL);
	AG_SetEvent(com, "circuit-hidden", hidden, NULL);
	AG_SetTimeout(&com->redraw_to, redraw, NULL, AG_TIMEOUT_LOADABLE);
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
	if (dip->p1 == dip->p2)
		fatal("p1 == p2");
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
static struct component *sel_com;

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
component_insert(int argc, union evarg *argv)
{
	char name[AG_OBJECT_NAME_MAX];
	AG_Mapview *mv = argv[1].p;
	AG_Tlist *tl = argv[2].p;
	struct circuit *ckt = argv[3].p;
	AG_TlistItem *it;
	AG_ObjectType *comtype;
	struct component_ops *comops;
	struct component *com;
	AG_Maptool *t;
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
	AG_PostEvent(ckt, com, "circuit-shown", NULL);

	if ((t = AG_MapviewFindTool(mv, "Insert component")) != NULL) {
		AG_MapviewSelectTool(mv, t, ckt);
	}
	sel_com = com;
	sel_com->selected = 1;

	AG_WidgetFocus(mv);
	AG_WindowFocus(AG_WidgetParentWindow(mv));
}

/* Remove the selected components from the circuit. */
static int
remove_component(AG_Maptool *t, SDLKey key, int state, void *arg)
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
	AG_Maptool *t = p;
	extern const struct eda_type eda_models[];
	const struct eda_type *ty;
	struct circuit *ckt = t->p;
	AG_Window *win;
	AG_Box *box;
	AG_Tlist *tl;
	int i;

	win = AG_MaptoolWindow(t, "circuit-component-tool");
	tl = AG_TlistNew(win, 0);
	AG_SetEvent(tl, "tlist-dblclick", component_insert, "%p, %p, %p",
	    t->mv, tl, ckt);

	box = AG_BoxNew(win, AG_BOX_HORIZ, AG_BOX_WFILL|AG_BOX_HOMOGENOUS);
	{
		AG_Button *bu;

		bu = AG_ButtonNew(box, _("Insert"));
		AG_SetEvent(bu, "button-pushed", component_insert, "%p, %p, %p",
		    t->mv, tl, ckt);
	}
	
	for (ty = &eda_models[0]; ty->name != NULL; ty++) {
		for (i = 0; i < agnTypes; i++) {
			if (strcmp(agTypes[i].type, ty->name) == 0)
				break;
		}
		if (i < agnTypes) {
			AG_ObjectType *ctype = &agTypes[i];
			struct component_ops *cops = (struct component_ops *)
			    ctype->ops;

			AG_TlistAddPtr(tl, AGICON(EDA_COMPONENT_ICON),
			    _(cops->name), ctype);
		}
	}

	sel_com = NULL;
	AG_MaptoolBindKey(t, KMOD_NONE, SDLK_DELETE, remove_component, NULL);
	AG_MaptoolBindKey(t, KMOD_NONE, SDLK_d, remove_component, NULL);
	AG_MaptoolPushStatus(t, _("Specify the type of component to create."));
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

	dprintf("rotate %s\n", AGOBJECT(com)->name);
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
ins_tool_buttondown(void *p, int xmap, int ymap, int b)
{
	AG_Maptool *t = p;
	struct circuit *ckt = t->p;
	VG *vg = ckt->vg;
	VG_Vtx vtx;
	int i;
	
	switch (b) {
	case SDL_BUTTON_LEFT:
		if (sel_com != NULL) {
			VG_Map2Vec(vg, xmap, ymap, &vtx.x, &vtx.y);
			component_connect(ckt, sel_com, &vtx);
			unselect_all_components(ckt);
			select_component(sel_com);
			sel_com = NULL;
			AG_MapviewSelectTool(t->mv, NULL, NULL);
		} else {
			dprintf("no component to connect\n");
		}
		break;
	case SDL_BUTTON_MIDDLE:
		if (sel_com != NULL) {
			rotate_component(ckt, sel_com);
		} else {
			dprintf("no component to rotate\n");
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
		AG_MapviewSelectTool(t->mv, NULL, NULL);
	}
	return (1);
}

/* Select overlapping components. */
static int
sel_tool_mousebuttondown(void *p, int xmap, int ymap, int b)
{
	AG_Maptool *t = p;
	struct circuit *ckt = t->p;
	VG *vg = ckt->vg;
	struct component *com;
	double x, y;
	int multi = SDL_GetModState() & KMOD_CTRL;
	AG_Window *pwin;

	if (b != SDL_BUTTON_LEFT)
		return (0);

	VG_Map2Vec(vg, xmap, ymap, &x, &y);

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
	if ((pwin = AG_WidgetParentWindow(t->mv)) != NULL) {
		agView->focus_win = pwin;
		AG_WidgetFocus(t->mv);
	}
	return (1);
}

/* Select overlapping components. */
static int
sel_tool_leftbutton(AG_Maptool *t, int button, int state, int rx, int ry,
    void *arg)
{
	struct circuit *ckt = t->p;
	VG *vg = ckt->vg;
	struct component *com;
	double x = VG_VECXF(vg,rx);
	double y = VG_VECYF(vg,ry);
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
	AG_Maptool *t = p;

	AG_MaptoolBindMouseButton(t, SDL_BUTTON_LEFT, sel_tool_leftbutton, NULL);
}

/* Move a floating component and highlight the overlapping nodes. */
static int
ins_tool_mousemotion(void *p, int xmap, int ymap, int xrel, int yrel, int b)
{
	AG_Maptool *t = p;
	struct circuit *ckt = t->p;
	VG *vg = ckt->vg;
	double x, y;
	struct pin *pin;
	int nconn;

	VG_Map2Vec(vg, xmap, ymap, &x, &y);
	vg->origin[1].x = x;
	vg->origin[1].y = y;

	if (sel_com != NULL) {
		VG_MoveBlock(vg, sel_com->block, x, y, -1);
		nconn = component_highlight_pins(ckt, sel_com);
		AG_MaptoolPopStatus(t);
		AG_MaptoolPushStatus(t, _("Connect %s (%d connections)"),
		    AGOBJECT(sel_com)->name, nconn);
		vg->redraw++;
		return (1);
	}
	return (0);
}

/* Highlight any component underneath the cursor. */
static int
sel_tool_mousemotion(void *p, int xmap, int ymap, int xrel, int yrel, int b)
{
	AG_Maptool *t = p;
	struct circuit *ckt = t->p;
	VG *vg = ckt->vg;
	struct component *com;
	double x, y;

	VG_Map2Vec(vg, xmap, ymap, &x, &y);
	vg->origin[1].x = x;
	vg->origin[1].y = y;

	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		VG_Rect rext;

		if (!AGOBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING)
			continue;

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

AG_MaptoolOps component_ins_tool = {
	N_("Insert component"),
	N_("Insert new components into the circuit."),
	EDA_INSERT_COMPONENT_ICON,
	sizeof(AG_Maptool),
	TOOL_HIDDEN,
	ins_tool_init,
	NULL,			/* destroy */
	NULL,			/* edit_pane */
	NULL,			/* edit */
	NULL,			/* cursor */
	NULL,			/* effect */
	ins_tool_mousemotion,
	ins_tool_buttondown,
	NULL,			/* mousebuttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};

AG_MaptoolOps component_sel_tool = {
	N_("Select component"),
	N_("Select one or more component in the circuit."),
	EDA_COMPONENT_ICON,
	sizeof(AG_Maptool),
	TOOL_HIDDEN,
	sel_tool_init,
	NULL,			/* destroy */
	NULL,			/* edit_pane */
	NULL,			/* edit */
	NULL,			/* cursor */
	NULL,			/* effect */
	sel_tool_mousemotion,
	sel_tool_mousebuttondown,
	NULL,			/* mousebuttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};
#endif /* EDITION */

#ifdef EDITION
void
component_reg_menu(AG_Menu *m, AG_MenuItem *pitem,
    struct circuit *ckt, AG_Mapview *mv)
{
	/* Nothing yet */
}
#endif /* EDITION */
