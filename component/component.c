/*	$Csoft: component.c,v 1.4 2005/09/10 05:48:21 vedge Exp $	*/

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
#include <engine/view.h>
#include <engine/typesw.h>
#include <engine/objmgr.h>

#include <engine/map/map.h>

#include <engine/vg/vg.h>
#include <engine/vg/vg_math.h>

#ifdef EDITION
#include <engine/widget/window.h>
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

#include <circuit/circuit.h>
#include <component/vsource.h>
#include <component/conductor.h>

#include "../tool.h"

const struct version component_ver = {
	"agar-eda component",
	0, 0
};

#define COMPONENT_REDRAW_DELAY	50

struct tool_ops component_ins_tool;
struct tool_ops component_sel_tool;

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
	objmgr_open_data(com, 1);
}

static void
unselect_component(struct component *com)
{
	com->selected = 0;
	objmgr_close_data(com);
}
		
static void
unselect_all_components(struct circuit *ckt)
{
	struct component *com;

	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!OBJECT_SUBCLASS(com, "component")) {
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

	object_copy_name(com, com->block->name, sizeof(com->block->name));
}

static void
attached(int argc, union evarg *argv)
{
	char blkname[VG_BLOCK_NAME_MAX];
	struct component *com = argv[0].p;
	struct circuit *ckt = argv[argc].p;
	struct vg *vg = ckt->vg;
	struct vg_block *block;

	if (!OBJECT_SUBCLASS(ckt, "circuit")) {
		return;
	}
	com->ckt = ckt;
	
	object_copy_name(com, blkname, sizeof(blkname));
#ifdef DEBUG
	if (com->block != NULL)
		fatal("block exists");
	if (vg_get_block(vg, blkname) != NULL)
		fatal("duplicate block: %s", blkname);
#endif
	com->block = vg_begin_block(vg, blkname, 0);
	vg_origin2(vg, 0, com->pin[0].x, com->pin[0].y);
	vg_end_block(vg);

	if (OBJECT_SUBCLASS(com, "component.vsource")) {
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
	int i;

	if (!OBJECT_SUBCLASS(ckt, "circuit"))
		return;
	
	if (OBJECT_SUBCLASS(com, "component.vsource")) {
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
	
	for (i = 1; i <= com->npins; i++) {
		struct pin *pin = &com->pin[i];

		if (pin->branch != NULL) {
			circuit_del_branch(com->ckt, pin->node, pin->branch);
			pin->branch = NULL;
			pin->node = -1;
		}
		pin->selected = 0;
	}

	for (i = 0; i < com->ndipoles; i++)
		com->dipoles[i].nloops = 0;

	if (com->block != NULL) {
		vg_destroy_block(ckt->vg, com->block);
		com->block = NULL;
	}
	com->ckt = NULL;
}

static void
shown(int argc, union evarg *argv)
{
	struct component *com = argv[0].p;

	timeout_add(com, &com->redraw_to, COMPONENT_REDRAW_DELAY);
}

static void
hidden(int argc, union evarg *argv)
{
	struct component *com = argv[0].p;

	lock_timeout(com);
	if (timeout_scheduled(com, &com->redraw_to)) {
		timeout_del(com, &com->redraw_to);
	}
	unlock_timeout(com);
}

/* Redraw a component schematic block. */
static Uint32
redraw(void *p, Uint32 ival, void *arg)
{
	struct component *com = p;
	struct vg_block *block_save;
	struct vg_element *element_save;
	struct vg *vg = com->ckt->vg;
	struct vg_element *vge;
	int i;
	
	if ((OBJECT(com)->flags & OBJECT_DATA_RESIDENT) == 0)
		return (ival);

#ifdef DEBUG
	if (com->block == NULL)
		fatal("%s: no block", OBJECT(com)->name);
#endif

	pthread_mutex_lock(&vg->lock);
	block_save = vg->cur_block;
	element_save = vg->cur_vge;
	vg_select_block(vg, com->block);
	vg_clear_block(vg, com->block);

	if (com->ops->draw != NULL)
		com->ops->draw(com, vg);

	/* Indicate the nodes associated with connection points. */
	for (i = 1; i <= com->npins; i++) {
		struct pin *pin = &com->pin[i];
		double rho, theta;
		double x, y;

		vg_car2pol(vg, pin->x, pin->y, &rho, &theta);
		theta -= com->block->theta;
		vg_pol2car(vg, rho, theta, &x, &y);

		if (com->ckt->dis_nodes) {
			vg_begin_element(vg, VG_CIRCLE);
			vg_vertex2(vg, x, y);
			if (pin->selected) {
				vg_color3(vg, 255, 255, 165);
				vg_circle_radius(vg, 0.1600);
			} else {
				vg_color3(vg, 0, 150, 150);
				vg_circle_radius(vg, 0.0625);
			}
			vg_end_element(vg);
		}
		if (com->ckt->dis_node_names &&
		    pin->node >= 0) {
			vg_begin_element(vg, VG_TEXT);
			vg_vertex2(vg, x, y);
			vg_style(vg, "node-name");
			vg_color3(vg, 0, 200, 100);
			vg_text_align(vg, VG_ALIGN_BR);
			vg_printf(vg, "n%d", pin->node+1);
			vg_end_element(vg);
		}
	}

	if (com->selected) {
		TAILQ_FOREACH(vge, &com->block->vges, vgbmbs) {
			vg_select_element(vg, vge);
			/* TODO blend */
			vg_color3(vg, 240, 240, 50);
			vg_end_element(vg);
		}
	} else if (com->highlighted) {
		TAILQ_FOREACH(vge, &com->block->vges, vgbmbs) {
			vg_select_element(vg, vge);
			/* TODO blend */
			vg_color3(vg, 180, 180, 120);
			vg_end_element(vg);
		}
	}

	if (com->block->theta != 0) {
		vg_rotate_block(vg, com->block, com->block->theta);
	}
	vg_select_element(vg, element_save);
	vg_select_block(vg, block_save);
	pthread_mutex_unlock(&vg->lock);
	return (ival);
}

void
component_init(void *obj, const char *tname, const char *name, const void *ops,
    const struct pin *pinout)
{
	char cname[OBJECT_NAME_MAX];
	struct component *com = obj;
	const struct pin *pinoutp;
	struct dipole *dip;
	int i, j, k;

	strlcpy(cname, "component.", sizeof(cname));
	strlcat(cname, tname, sizeof(cname));
	object_init(com, cname, name, ops);
	com->ops = ops;
	com->ckt = NULL;
	com->block = NULL;
	com->npins = 0;
	com->selected = 0;
	com->highlighted = 0;
	com->Tnom = 27+273.15;
	com->Tspec = 27+273.15;
	pthread_mutex_init(&com->lock, &recursive_mutexattr);

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

	event_new(com, "attached", attached, NULL);
	event_new(com, "detached", detached, NULL);
	event_new(com, "renamed", renamed, NULL);
	event_new(com, "circuit-shown", shown, NULL);
	event_new(com, "circuit-hidden", hidden, NULL);
	timeout_set(&com->redraw_to, redraw, NULL, TIMEOUT_LOADABLE);
}

int
component_load(void *p, struct netbuf *buf)
{
	struct component *com = p;
	float Tnom, Tspec;

	if (version_read(buf, &component_ver, NULL) == -1)
		return (-1);

	com->flags = (u_int)read_uint32(buf);
	com->Tnom = read_float(buf);
	com->Tspec = read_float(buf);
	return (0);
}

int
component_save(void *p, struct netbuf *buf)
{
	struct component *com = p;

	version_write(buf, &component_ver);
	write_uint32(buf, com->flags);
	write_float(buf, com->Tnom);
	write_float(buf, com->Tspec);
	return (0);
}

/* Return the effective DC resistance between two dipoles. */
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

/* Return the effective capacitance between two dipoles. */
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

/* Return the effective inductance between two dipoles. */
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

struct window *
component_edit(void *p)
{
	struct component *com = p;
	struct window *win;

	if (com->ops->edit == NULL) {
		return (NULL);
	}
	win = com->ops->edit(com);
	window_set_position(win, WINDOW_MIDDLE_LEFT, 0);
	window_set_caption(win, "%s: %s", com->ops->name, OBJECT(com)->name);
	window_set_closure(win, WINDOW_DETACH);
	window_show(win);
	return (win);
}

/* Insert a new component into a circuit. */
void
component_insert(int argc, union evarg *argv)
{
	char name[OBJECT_NAME_MAX];
	struct mapview *mv = argv[1].p;
	struct tlist *tl = argv[2].p;
	struct circuit *ckt = argv[3].p;
	struct tlist_item *it;
	struct object_type *comtype;
	struct component_ops *comops;
	struct component *com;
	struct tool *t;
	int n = 1;

	if ((it = tlist_selected_item(tl)) == NULL) {
		text_msg(MSG_ERROR, _("No component type is selected."));
		return;
	}
	comtype = it->p1;
	comops = (struct component_ops *)comtype->ops;
tryname:
	snprintf(name, sizeof(name), "%s%d", comops->pfx, n++);
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (strcmp(OBJECT(com)->name, name) == 0)
			break;
	}
	if (com != NULL)
		goto tryname;

	com = Malloc(comtype->size, M_OBJECT);
	comtype->ops->init(com, name);
	com->flags |= COMPONENT_FLOATING;
	object_attach(ckt, com);
	object_unlink_datafiles(com);
	object_page_in(com, OBJECT_DATA);
	event_post(ckt, com, "circuit-shown", NULL);

	if ((t = mapview_find_tool(mv, "Insert component")) != NULL) {
		mapview_select_tool(mv, t, ckt);
	}
	sel_com = com;
	sel_com->selected = 1;

	widget_focus(mv);
	window_focus(widget_parent_window(mv));
}

/* Remove the selected components from the circuit. */
static int
remove_component(struct tool *t, SDLKey key, int state, void *arg)
{
	struct circuit *ckt = t->p;
	struct component *com;

	if (!state) {
		return (0);
	}
	lock_linkage();
scan:
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!OBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING ||
		    !com->selected) {
			continue;
		}
		if (OBJECT_SUBCLASS(com, "component.conductor")) {
			conductor_tool_reinit();
		}
		objmgr_close_data(com);
		if (object_in_use(com)) {
			text_msg(MSG_ERROR, "%s: %s", OBJECT(com)->name,
			    error_get());
			continue;
		}
		object_detach(com);
		object_unlink_datafiles(com);
		object_destroy(com);
		Free(com, M_OBJECT);
		goto scan;
	}
	unlock_linkage();
	circuit_modified(ckt);
	return (1);
}

static void
ins_tool_init(void *p)
{
	struct tool *t = p;
	extern const struct eda_type eda_models[];
	const struct eda_type *ty;
	struct circuit *ckt = t->p;
	struct window *win;
	struct box *box;
	struct tlist *tl;
	int i;

	win = tool_window(t, "circuit-component-tool");
	tl = tlist_new(win, 0);
	event_new(tl, "tlist-dblclick", component_insert, "%p, %p, %p",
	    t->mv, tl, ckt);

	box = box_new(win, BOX_HORIZ, BOX_WFILL|BOX_HOMOGENOUS);
	{
		struct button *bu;

		bu = button_new(box, _("Insert"));
		event_new(bu, "button-pushed", component_insert, "%p, %p, %p",
		    t->mv, tl, ckt);
	}
	
	for (ty = &eda_models[0]; ty->name != NULL; ty++) {
		for (i = 0; i < ntypesw; i++) {
			if (strcmp(typesw[i].type, ty->name) == 0)
				break;
		}
		if (i < ntypesw) {
			struct object_type *ctype = &typesw[i];
			struct component_ops *cops = (struct component_ops *)
			    ctype->ops;

			tlist_insert_item(tl, ICON(EDA_COMPONENT_ICON),
			    _(cops->name), ctype);
		}
	}

	sel_com = NULL;
	tool_bind_key(t, KMOD_NONE, SDLK_DELETE, remove_component, NULL);
	tool_bind_key(t, KMOD_NONE, SDLK_d, remove_component, NULL);
	tool_push_status(t, _("Specify the type of component to create."));
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
	OBJECT_FOREACH_CHILD(ocom, ckt, component) {
		if (!OBJECT_SUBCLASS(ocom, "component") ||
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

/* Connect a floating component's pins to overlapping nodes. */
void
component_connect(struct circuit *ckt, struct component *com,
    struct vg_vertex *vtx)
{
	struct pin *opin;
	struct cktbranch *br;
	int i;

	for (i = 1; i <= com->npins; i++) {
		struct pin *pin = &com->pin[i];

		opin = pin_overlap(ckt, com,
		    vtx->x + pin->x,
		    vtx->y + pin->y);
		if (opin != NULL) {
			if (pin->node >= 0) {
				circuit_del_node(ckt, pin->node);
			}
			pin->node = opin->node;
			pin->branch = circuit_add_branch(ckt, opin->node,
			    &com->pin[i]);
		} else {
			pin->node = circuit_add_node(ckt, 0);
			pin->branch = circuit_add_branch(ckt, pin->node,
			    &com->pin[i]);
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
	struct vg *vg = ckt->vg;
	int i;

	dprintf("rotate %s\n", OBJECT(com)->name);
	com->block->theta += M_PI_2;

	for (i = 1; i <= com->npins; i++) {
		double rho, theta;

		vg_select_block(vg, com->block);
		vg_car2pol(vg, com->pin[i].x, com->pin[i].y, &rho, &theta);
		theta += M_PI_2;
		vg_pol2car(vg, rho, theta, &com->pin[i].x, &com->pin[i].y);
		vg_end_block(vg);
	}
}

/* Enter component initialization parameters. */
static int
configure_component(struct component *com)
{
	if (com->ops->configure == NULL) {
		return (0);
	}
	return (com->ops->configure(com));
}

static int
ins_tool_buttondown(void *p, int xmap, int ymap, int b)
{
	struct tool *t = p;
	struct circuit *ckt = t->p;
	struct vg *vg = ckt->vg;
	struct vg_vertex vtx;
	int i;
	
	switch (b) {
	case SDL_BUTTON_LEFT:
		if (sel_com != NULL) {
			vg_map2vec(vg, xmap, ymap, &vtx.x, &vtx.y);
			component_connect(ckt, sel_com, &vtx);
			if (configure_component(sel_com) == 0) {
				unselect_all_components(ckt);
				select_component(sel_com);
				sel_com = NULL;
				mapview_select_tool(t->mv, NULL, NULL);
			} else {
				goto remove;
			}
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
		object_detach(sel_com);
		object_unlink_datafiles(sel_com);
		object_destroy(sel_com);
		Free(sel_com, M_OBJECT);
		sel_com = NULL;
		mapview_select_tool(t->mv, NULL, NULL);
	}
	return (1);
}

/* Select overlapping components. */
static int
sel_tool_mousebuttondown(void *p, int xmap, int ymap, int b)
{
	struct tool *t = p;
	struct circuit *ckt = t->p;
	struct vg *vg = ckt->vg;
	struct component *com;
	double x, y;
	int multi = SDL_GetModState() & KMOD_CTRL;
	struct window *pwin;

	if (b != SDL_BUTTON_LEFT)
		return (0);

	vg_map2vec(vg, xmap, ymap, &x, &y);

	if (!multi) {
		unselect_all_components(ckt);
	}
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		struct vg_rect rext;

		if (!OBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		vg_block_extent(vg, com->block, &rext);
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
	if ((pwin = widget_parent_window(t->mv)) != NULL) {
		view->focus_win = pwin;
		widget_focus(t->mv);
	}
	return (1);
}

/* Select overlapping components. */
static int
sel_tool_leftbutton(struct tool *t, int button, int state, int rx, int ry,
    void *arg)
{
	struct circuit *ckt = t->p;
	struct vg *vg = ckt->vg;
	struct component *com;
	double x = VG_VECXF(vg,rx);
	double y = VG_VECYF(vg,ry);
	int multi = SDL_GetModState() & KMOD_CTRL;

	if (button != SDL_BUTTON_LEFT || !state)
		return (0);

	if (!multi) {
		unselect_all_components(ckt);
	}
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		struct vg_rect rext;

		if (!OBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		vg_block_extent(vg, com->block, &rext);
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
	struct tool *t = p;

	tool_bind_mousebutton(t, SDL_BUTTON_LEFT, sel_tool_leftbutton, NULL);
}

/* Move a floating component and highlight the overlapping nodes. */
static int
ins_tool_mousemotion(void *p, int xmap, int ymap, int xrel, int yrel, int b)
{
	struct tool *t = p;
	struct circuit *ckt = t->p;
	struct vg *vg = ckt->vg;
	double x, y;
	struct pin *pin;
	int nconn;

	vg_map2vec(vg, xmap, ymap, &x, &y);
	vg->origin[1].x = x;
	vg->origin[1].y = y;

	if (sel_com != NULL) {
		vg_move_block(vg, sel_com->block, x, y, -1);
		nconn = component_highlight_pins(ckt, sel_com);
		tool_pop_status(t);
		tool_push_status(t, _("Connect %s (%d connections)"),
		    OBJECT(sel_com)->name, nconn);
		vg->redraw++;
		return (1);
	}
	return (0);
}

/* Highlight any component underneath the cursor. */
static int
sel_tool_mousemotion(void *p, int xmap, int ymap, int xrel, int yrel, int b)
{
	struct tool *t = p;
	struct circuit *ckt = t->p;
	struct vg *vg = ckt->vg;
	struct component *com;
	double x, y;

	vg_map2vec(vg, xmap, ymap, &x, &y);
	vg->origin[1].x = x;
	vg->origin[1].y = y;

	OBJECT_FOREACH_CHILD(com, ckt, component) {
		struct vg_rect rext;

		if (!OBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING)
			continue;

		vg_block_extent(vg, com->block, &rext);
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

struct tool_ops component_ins_tool = {
	N_("Insert component"),
	N_("Insert new components into the circuit."),
	EDA_INSERT_COMPONENT_ICON,
	sizeof(struct tool),
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

struct tool_ops component_sel_tool = {
	N_("Select component"),
	N_("Select one or more component in the circuit."),
	EDA_COMPONENT_ICON,
	sizeof(struct tool),
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
component_reg_menu(struct AGMenu *m, struct AGMenuItem *pitem,
    struct circuit *ckt, struct mapview *mv)
{
	/* Nothing yet */
}
#endif /* EDITION */
