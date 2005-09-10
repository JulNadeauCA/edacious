/*	$Csoft: circuit.c,v 1.5 2005/09/09 02:50:14 vedge Exp $	*/

/*
 * Copyright (c) 2004, 2005 CubeSoft Communications Inc
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
#include <engine/typesw.h>

#include <engine/map/map.h>

#ifdef EDITION
#include <engine/widget/gui.h>
#include <engine/map/mapview.h>
#include <engine/objmgr.h>
#endif

#include <math.h>

#include <component/vsource.h>

#include "tool.h"
#include "circuit.h"
#include "spice.h"

const struct version circuit_ver = {
	"agar-eda circuit",
	0, 0
};

const struct object_ops circuit_ops = {
	circuit_init,
	circuit_reinit,
	circuit_destroy,
	circuit_load,
	circuit_save,
	circuit_edit
};

static void
circuit_detached(int argc, union evarg *argv)
{
	struct circuit *ckt = argv[0].p;

	if (ckt->sim != NULL) {
		sim_destroy(ckt->sim);
		ckt->sim = NULL;
	}
}

static void
circuit_opened(int argc, union evarg *argv)
{
	struct circuit *ckt = argv[0].p;
	struct component *com;

	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!OBJECT_SUBCLASS(com, "component"))
			continue;

		event_post(ckt, com, "circuit-shown", NULL);
		object_page_in(com, OBJECT_DATA);
	}
}

static void
circuit_closed(int argc, union evarg *argv)
{
	struct circuit *ckt = argv[0].p;
	struct component *com;

	if (ckt->sim != NULL) {
		sim_destroy(ckt->sim);
		ckt->sim = NULL;
	}

	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!OBJECT_SUBCLASS(com, "component"))
			continue;

		event_post(ckt, com, "circuit-hidden", NULL);
		object_page_out(com, OBJECT_DATA);
	}
}

/* Effect a change in the circuit topology.  */
void
circuit_modified(struct circuit *ckt)
{
	struct component *com;
	struct vsource *vs;
	struct cktloop *loop;
	unsigned int i;

	/* Regenerate loop and dipole information. */
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!OBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING)
			continue;

		for (i = 0; i < com->ndipoles; i++)
			com->dipoles[i].nloops = 0;
	}
	OBJECT_FOREACH_CHILD(vs, ckt, vsource) {
		if (!OBJECT_SUBCLASS(vs, "component.vsource") ||
		    COM(vs)->flags & COMPONENT_FLOATING ||
		    PIN(vs,1)->node == NULL ||
		    PIN(vs,2)->node == NULL) {
			continue;
		}
		vsource_free_loops(vs);
		vsource_find_loops(vs);
	}
#if 0
	OBJECT_FOREACH_CHILD(com, ckt, component)
		event_post(ckt, com, "circuit-modified", NULL);
#endif
	if (ckt->loops == NULL) {
		ckt->loops = Malloc(sizeof(struct cktloop *), M_EDA);
	}
	ckt->l = 0;
	OBJECT_FOREACH_CHILD(vs, ckt, vsource) {
		if (!OBJECT_SUBCLASS(vs, "component.vsource") ||
		    COM(vs)->flags & COMPONENT_FLOATING) {
			continue;
		}
		TAILQ_FOREACH(loop, &vs->loops, loops) {
			ckt->loops = Realloc(ckt->loops,
			    (ckt->l+1)*sizeof(struct cktloop *));
			ckt->loops[ckt->l++] = loop;
			loop->name = ckt->l;
		}
	}

	if (ckt->sim != NULL &&
	    ckt->sim->ops->cktmod != NULL)
		ckt->sim->ops->cktmod(ckt->sim, ckt);
}

void
circuit_init(void *p, const char *name)
{
	struct circuit *ckt = p;
	struct vg_style *vgs;
	struct vg *vg;

	object_init(ckt, "circuit", name, &circuit_ops);
	ckt->sim = NULL;
	ckt->dis_nodes = 1;
	ckt->dis_node_names = 1;

	ckt->nodes = NULL;
	ckt->loops = NULL;
	ckt->vsrcs = NULL;
	ckt->n = 0;
	ckt->m = 0;
	ckt->n = 0;
	pthread_mutex_init(&ckt->lock, &recursive_mutexattr);

	vg = ckt->vg = vg_new(ckt, VG_VISGRID);
	strlcpy(vg->layers[0].name, _("Schematic"), sizeof(vg->layers[0].name));
	vg_scale(vg, 11.0, 8.5, 2.0);
	vg_default_scale(vg, 2.0);
	vg_grid_gap(vg, 0.25);
	vg->origin[0].x = 5.5;
	vg->origin[0].y = 4.25;
	vg_origin_radius(vg, 2, 0.09375);
	vg_origin_color(vg, 2, 255, 255, 165);

	vgs = vg_create_style(vg, VG_TEXT_STYLE, "component-name");
	vgs->vg_text_st.size = 10;
	
	vgs = vg_create_style(vg, VG_TEXT_STYLE, "node-name");
	vgs->vg_text_st.size = 8;
	
	event_new(ckt, "edit-open", circuit_opened, NULL);
	event_new(ckt, "edit-close", circuit_closed, NULL);
}

void
circuit_reinit(void *p)
{
	struct circuit *ckt = p;
	struct component *com;
	int i;
	
	if (ckt->sim != NULL) {
		sim_destroy(ckt->sim);
		ckt->sim = NULL;
	}

	Free(ckt->loops, M_EDA);
	ckt->loops = NULL;
	ckt->l = 0;

	Free(ckt->vsrcs, M_EDA);
	ckt->vsrcs = NULL;
	ckt->m = 0;
	
	for (i = 0; i < ckt->n; i++) {
		circuit_destroy_node(ckt->nodes[i]);
		Free(ckt->nodes[i], M_EDA);
	}
	Free(ckt->nodes, M_EDA);
	ckt->nodes = NULL;
	ckt->n = 0;

	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!OBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		for (i = 1; i <= com->npins; i++) {
			struct pin *pin = &com->pin[i];

			pin->node = NULL;
			pin->branch = NULL;
			pin->selected = 0;
			pin->u = 0;
			pin->du = 0;
		}
		com->block = NULL;
	}
	vg_reinit(ckt->vg);
}

int
circuit_load(void *p, struct netbuf *buf)
{
	struct circuit *ckt = p;
	Uint32 i, ncoms, nnodes;
	int j;

	if (version_read(buf, &circuit_ver, NULL) != 0) {
		return (-1);
	}
	copy_string(ckt->descr, buf, sizeof(ckt->descr));
	read_uint32(buf);					/* Pad */

	/* Load the circuit nodes. */
	nnodes = read_uint32(buf);
	for (i = 0; i < nnodes; i++) {
		int nbranches, flags, name;

		flags = (int)read_uint32(buf);
		nbranches = (int)read_uint32(buf);
		name = circuit_add_node(ckt, flags & ~(CKTNODE_EXAM));
		dprintf("node %d/%d: flags=0x%x, branches=%d\n", i, ckt->n,
		    flags, nbranches);
		for (j = 0; j < nbranches; j++) {
			char ppath[OBJECT_PATH_MAX];
			struct component *pcom;
			struct cktbranch *br;
			int ppin;

			copy_string(ppath, buf, sizeof(ppath));
			read_uint32(buf);			/* Pad */
			ppin = (int)read_uint32(buf);
			if ((pcom = object_find(ppath)) == NULL) {
				error_set("%s", error_get());
				return (-1);
			}
			if (ppin > pcom->npins) {
				error_set("%s: pin #%d > %d", ppath, ppin,
				    pcom->npins);
				return (-1);
			}
			br = circuit_add_branch(ckt, name, &pcom->pin[ppin]);
			pcom->pin[ppin].node = name;
			pcom->pin[ppin].branch = br;
		}
	}

	/* Load the schematics. */
	if (vg_load(ckt->vg, buf) == -1)
		return (-1);

	/* Load the component pin assignments. */
	ncoms = read_uint32(buf);
	for (i = 0; i < ncoms; i++) {
		char name[OBJECT_NAME_MAX];
		char path[OBJECT_PATH_MAX];
		int j, npins;
		struct component *com;

		/* Lookup the component. */
		copy_string(name, buf, sizeof(name));
		OBJECT_FOREACH_CHILD(com, ckt, component) {
			if (OBJECT_SUBCLASS(com, "component") &&
			    strcmp(OBJECT(com)->name, name) == 0)
				break;
		}
		if (com == NULL)
			fatal("unexisting component: `%s'", name);
		
		/* Reassign the block pointer since the vg was reloaded. */
		object_copy_name(com, path, sizeof(path));
		if ((com->block = vg_get_block(ckt->vg, path)) == NULL)
			fatal("unexisting block: `%s'", path);

		/* Load the pinout information. */
		com->npins = (int)read_uint32(buf);
		for (j = 1; j <= com->npins; j++) {
			struct pin *pin = &com->pin[j];
			struct vg_block *block_save;
			struct cktbranch *br;
			struct cktnode *node;

			pin->n = (int)read_uint32(buf);
			copy_string(pin->name, buf, sizeof(pin->name));
			pin->x = read_double(buf);
			pin->y = read_double(buf);
			pin->node = (int)read_uint32(buf);
			node = ckt->nodes[pin->node];

			TAILQ_FOREACH(br, &node->branches, branches) {
				if (br->pin->com == com &&
				    br->pin->n == pin->n)
					break;
			}
			if (br == NULL) {
				fatal("no such branch: %s:%d",
				    OBJECT(com)->name, pin->n);
			}
			pin->branch = br;
			pin->selected = 0;
		}
	}
	circuit_modified(ckt);
	return (0);
}

int
circuit_save(void *p, struct netbuf *buf)
{
	char path[OBJECT_PATH_MAX];
	struct circuit *ckt = p;
	struct cktnode *node;
	struct cktbranch *br;
	struct component *com;
	off_t ncoms_offs;
	Uint32 i, ncoms = 0;

	version_write(buf, &circuit_ver);
	write_string(buf, ckt->descr);
	write_uint32(buf, 0);					/* Pad */
	write_uint32(buf, ckt->n);
	for (i = 0; i < ckt->n; i++) {
		struct cktnode *node = ckt->nodes[i];
		off_t nbranches_offs;
		Uint32 nbranches = 0;

		dprintf("save node %d/%d\n", i, ckt->n);
		write_uint32(buf, (Uint32)node->flags);
		nbranches_offs = netbuf_tell(buf);
		write_uint32(buf, 0);
		TAILQ_FOREACH(br, &node->branches, branches) {
			if (br->pin == NULL || br->pin->com == NULL ||
			    br->pin->com->flags & COMPONENT_FLOATING) {
				continue;
			}
			object_copy_name(br->pin->com, path, sizeof(path));
			write_string(buf, path);
			write_uint32(buf, 0);			/* Pad */
			write_uint32(buf, (Uint32)br->pin->n);
			dprintf("save branch %d (%s:%u)\n", nbranches, path,
			    br->pin->n);
			nbranches++;
		}
		pwrite_uint32(buf, nbranches, nbranches_offs);
	}
	
	/* Save the schematics. */
	vg_save(ckt->vg, buf);
	
	/* Save the component information. */
	ncoms_offs = netbuf_tell(buf);
	write_uint32(buf, 0);
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!OBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		dprintf("save component %s (%u pins)\n", OBJECT(com)->name,
		    com->npins);
		write_string(buf, OBJECT(com)->name);
		write_uint32(buf, (Uint32)com->npins);
		for (i = 1; i <= com->npins; i++) {
			struct pin *pin = &com->pin[i];

			write_uint32(buf, (Uint32)pin->n);
			write_string(buf, pin->name);
			write_double(buf, pin->x);
			write_double(buf, pin->y);
#ifdef DEBUG
			if (pin->node == -1)
				fatal("%s: bad pin %d", OBJECT(com)->name, i);
#endif
			write_uint32(buf, (Uint32)pin->node);
		}
		ncoms++;
	}
	pwrite_uint32(buf, ncoms, ncoms_offs);
	return (0);
}

static void
circuit_node_init(struct cktnode *node, u_int flags)
{
	node->flags = flags;
	node->nbranches = 0;
	TAILQ_INIT(&node->branches);
}

int
circuit_add_node(struct circuit *ckt, u_int flags)
{
	struct cktnode *node;

	ckt->nodes = Realloc(ckt->nodes, (ckt->n+1)*sizeof(struct cktnode *));
	ckt->nodes[ckt->n] = Malloc(sizeof(struct cktnode), M_EDA);
	circuit_node_init(ckt->nodes[ckt->n], flags);
	return (ckt->n++);
}

int
circuit_gnd_node(struct circuit *ckt, int n)
{
	struct cktbranch *br;

	TAILQ_FOREACH(br, &ckt->nodes[n]->branches, branches) {
		if (br->pin != NULL && br->pin->com != NULL &&
		    OBJECT_SUBCLASS(br->pin->com, "component.ground"))
			return (1);
	}
	return (0);
}

void
circuit_destroy_node(struct cktnode *node)
{
	struct cktbranch *br, *nbr;

	for (br = TAILQ_FIRST(&node->branches);
	     br != TAILQ_END(&node->branches);
	     br = nbr) {
		nbr = TAILQ_NEXT(br, branches);
		Free(br, M_EDA);
	}
	TAILQ_INIT(&node->branches);
}

void
circuit_del_node(struct circuit *ckt, int n)
{
	struct cktnode *node;
	struct cktbranch *br;
	int i;

	if (n < 0 || n >= ckt->n) {
		dprintf("no such node: n%d\n", n);
		return;
	}
	node = ckt->nodes[n];
	circuit_destroy_node(node);
	Free(node, M_EDA);

	if (n < --ckt->n) {
		for (i = n; i < ckt->n; i++) {
			ckt->nodes[i] = ckt->nodes[i+1];

			/* Update the branch node references. */
			TAILQ_FOREACH(br, &ckt->nodes[i]->branches, branches) {
				if (br->pin != NULL && br->pin->com != NULL)
					br->pin->node = i;
			}
		}
	}
}

struct cktbranch *
circuit_add_branch(struct circuit *ckt, int n, struct pin *pin)
{
	struct cktnode *node = ckt->nodes[n];
	struct cktbranch *br;

	br = Malloc(sizeof(struct cktbranch), M_EDA);
	br->pin = pin;
	TAILQ_INSERT_TAIL(&node->branches, br, branches);
	node->nbranches++;
	return (br);
}

struct cktbranch *
circuit_get_branch(struct circuit *ckt, int n, struct pin *pin)
{
	struct cktnode *node = ckt->nodes[n];
	struct cktbranch *br;

	TAILQ_FOREACH(br, &node->branches, branches) {
		if (br->pin == pin)
			break;
	}
	return (br);
}

void
circuit_del_branch(struct circuit *ckt, int n, struct cktbranch *br)
{
	struct cktnode *node = ckt->nodes[n];

	TAILQ_REMOVE(&node->branches, br, branches);
	Free(br, M_EDA);

	if (--node->nbranches == 0) {
		dprintf("removing node %d (tot %d)\n", n, ckt->n);
		circuit_del_node(ckt, n);
	}
}

void
circuit_free_components(struct circuit *ckt)
{
	struct component *com, *ncom;

	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!OBJECT_SUBCLASS(com, "component")) {
			continue;
		}
		object_detach(com);
		object_destroy(com);
		Free(com, M_OBJECT);
	}
}

void
circuit_destroy(void *p)
{
	struct circuit *ckt = p;
	
	vg_destroy(ckt->vg);
	pthread_mutex_destroy(&ckt->lock);
}

/* Select the simulation mode. */
struct sim *
circuit_set_sim(struct circuit *ckt, const struct sim_ops *sops)
{
	struct sim *sim;

	if (ckt->sim != NULL) {
		sim_destroy(ckt->sim);
		ckt->sim = NULL;
	}
	ckt->sim = sim = Malloc(sops->size, M_EDA);
	sim->ckt = ckt;
	sim->ops = sops;
	sim->running = 0;

	if (sim->ops->init != NULL) {
		sim->ops->init(sim);
	}
	if (sim->ops->edit != NULL &&
	   (sim->win = sim->ops->edit(sim, ckt)) != NULL) {
		window_set_caption(sim->win, "%s: %s", OBJECT(ckt)->name,
		    _(sim->ops->name));
		window_set_position(sim->win, WINDOW_LOWER_LEFT, 0);
		window_show(sim->win);
	}
	return (sim);
}

#ifdef EDITION

static void
rasterize_circuit(struct mapview *mv, void *p)
{
	struct vg *vg = p;

	if (vg->redraw) {
		vg->redraw = 0;
	}
	vg->redraw++;
	vg_rasterize(vg);
}

static void
poll_loops(int argc, union evarg *argv)
{
	struct tlist *tl = argv[0].p;
	struct circuit *ckt = argv[1].p;
	int i;

	tlist_clear_items(tl);
	for (i = 0; i < ckt->l; i++) {
		char voltage[32];
		struct cktloop *loop = ckt->loops[i];
		struct vsource *vs = (struct vsource *)loop->origin;
		struct tlist_item *it;

		unit_format(vs->voltage, emf_units, voltage, sizeof(voltage));
		it = tlist_insert(tl, NULL, "%s:L%u (%s)", OBJECT(vs)->name,
		    loop->name, voltage);
		it->p1 = loop;
	}
	tlist_restore_selections(tl);
}

static void
poll_nodes(int argc, union evarg *argv)
{
	struct tlist *tl = argv[0].p;
	struct circuit *ckt = argv[1].p;
	int i;

	tlist_clear_items(tl);
	for (i = 0; i < ckt->n; i++) {
		struct cktnode *node = ckt->nodes[i];
		struct cktbranch *br;
		struct tlist_item *it, *it2;

		it = tlist_insert(tl, NULL, "n%d (0x%x, %d branches)", i,
		    node->flags, node->nbranches);
		it->p1 = node;
		it->depth = 0;
		TAILQ_FOREACH(br, &node->branches, branches) {
			it = tlist_insert(tl, NULL, "%s:%s(%d)",
			    (br->pin != NULL && br->pin->com != NULL) ?
			    OBJECT(br->pin->com)->name : "(null)",
			    br->pin->name, br->pin->n);
			it->p1 = br;
			it->depth = 1;
		}
	}
	tlist_restore_selections(tl);
}

static void
poll_sources(int argc, union evarg *argv)
{
	struct tlist *tl = argv[0].p;
	struct circuit *ckt = argv[1].p;
	int i;

	tlist_clear_items(tl);
	for (i = 0; i < ckt->m; i++) {
		struct vsource *vs = ckt->vsrcs[i];
		struct tlist_item *it;

		it = tlist_insert(tl, NULL, "%s", OBJECT(vs)->name);
		it->p1 = vs;
	}
	tlist_restore_selections(tl);
}

static void
revert_circuit(int argc, union evarg *argv)
{
	struct circuit *ckt = argv[1].p;

	if (object_load(ckt) == 0) {
		text_tmsg(MSG_INFO, 1000,
		    _("Circuit `%s' reverted successfully."),
		    OBJECT(ckt)->name);
	} else {
		text_msg(MSG_ERROR, "%s: %s", OBJECT(ckt)->name,
		    error_get());
	}
}

static void
save_circuit(int argc, union evarg *argv)
{
	struct circuit *ckt = argv[1].p;

	if (object_save(world) == 0 &&
	    object_save(ckt) == 0) {
		text_tmsg(MSG_INFO, 1250,
		    _("Circuit `%s' saved successfully."),
		    OBJECT(ckt)->name);
	} else {
		text_msg(MSG_ERROR, "%s: %s", OBJECT(ckt)->name,
		    error_get());
	}
}

static void
show_interconnects(int argc, union evarg *argv)
{
	struct window *pwin = argv[1].p;
	struct circuit *ckt = argv[2].p;
	struct window *win;
	struct notebook *nb;
	struct notebook_tab *ntab;
	struct tlist *tl;
	
	if ((win = window_new(0, "%s-interconnects", OBJECT(ckt)->name))
	    == NULL) {
		return;
	}
	window_set_caption(win, _("%s: Connections"), OBJECT(ckt)->name);
	window_set_position(win, WINDOW_UPPER_RIGHT, 0);
	
	nb = notebook_new(win, NOTEBOOK_HFILL|NOTEBOOK_WFILL);
	ntab = notebook_add_tab(nb, _("Nodes"), BOX_VERT);
	{
		tl = tlist_new(ntab, TLIST_POLL|TLIST_TREE);
		event_new(tl, "tlist-poll", poll_nodes, "%p", ckt);
	}
	
	ntab = notebook_add_tab(nb, _("Loops"), BOX_VERT);
	{
		tl = tlist_new(ntab, TLIST_POLL);
		event_new(tl, "tlist-poll", poll_loops, "%p", ckt);
	}
	
	ntab = notebook_add_tab(nb, _("Sources"), BOX_VERT);
	{
		tl = tlist_new(ntab, TLIST_POLL);
		event_new(tl, "tlist-poll", poll_sources, "%p", ckt);
	}

	window_attach(pwin, win);
	window_show(win);
}

static void
layout_settings(int argc, union evarg *argv)
{
	char path[OBJECT_PATH_MAX];
	struct window *pwin = argv[1].p;
	struct circuit *ckt = argv[2].p;
	struct vg *vg = ckt->vg;
	struct window *win;
	struct mfspinbutton *mfsu;
	struct fspinbutton *fsu;
	struct textbox *tb;
	struct checkbox *cb;
	
	object_copy_name(ckt, path, sizeof(path));
	if ((win = window_new(0, "settings-%s", path)) == NULL) {
		return;
	}
	window_set_caption(win, _("Circuit layout: %s"), OBJECT(ckt)->name);
	window_set_position(win, WINDOW_UPPER_RIGHT, 0);

	label_new(win, LABEL_POLLED, _("Loops: %u"), &ckt->l);
	label_new(win, LABEL_POLLED, _("Voltage sources: %u"), &ckt->m);
	label_new(win, LABEL_POLLED, _("Nodes: %u"), &ckt->n);

	tb = textbox_new(win, _("Description: "));
	widget_bind(tb, "string", WIDGET_STRING, &ckt->descr,
	    sizeof(ckt->descr));

	mfsu = mfspinbutton_new(win, NULL, "x", _("Bounding box: "));
	WIDGET(mfsu)->flags |= WIDGET_WFILL;
	widget_bind(mfsu, "xvalue", WIDGET_DOUBLE, &vg->w);
	widget_bind(mfsu, "yvalue", WIDGET_DOUBLE, &vg->h);
	mfspinbutton_set_min(mfsu, 1.0);
	mfspinbutton_set_increment(mfsu, 0.1);
	event_new(mfsu, "mfspinbutton-changed", vg_geo_changed, "%p", vg);

	fsu = fspinbutton_new(win, NULL, _("Grid interval: "));
	WIDGET(fsu)->flags |= WIDGET_WFILL;
	widget_bind(fsu, "value", WIDGET_DOUBLE, &vg->grid_gap);
	fspinbutton_set_min(fsu, 0.0625);
	fspinbutton_set_increment(fsu, 0.0625);
	event_new(fsu, "fspinbutton-changed", vg_changed, "%p", vg);
	
	fsu = fspinbutton_new(win, NULL, _("Scaling factor: "));
	WIDGET(fsu)->flags |= WIDGET_WFILL;
	widget_bind(fsu, "value", WIDGET_DOUBLE, &vg->scale);
	fspinbutton_set_min(fsu, 0.1);
	fspinbutton_set_increment(fsu, 0.1);
	event_new(fsu, "fspinbutton-changed", vg_geo_changed, "%p", vg);

	cb = checkbox_new(win, _("Display node indicators"));
	widget_bind(cb, "state", WIDGET_INT, &ckt->dis_nodes);

	cb = checkbox_new(win, _("Display node names"));
	widget_bind(cb, "state", WIDGET_INT, &ckt->dis_node_names);
	
#if 0
	label_new(win, LABEL_STATIC, _("Nodes:"));
	tl = tlist_new(win, TLIST_POLL|TLIST_TREE);
	WIDGET(tl)->flags &= ~(WIDGET_HFILL);
	event_new(tl, "tlist-poll", poll_nodes, "%p", ckt);
#endif	

	window_attach(pwin, win);
	window_show(win);
}

static void
export_to_spice(int argc, union evarg *argv)
{
	char name[FILENAME_MAX];
	struct circuit *ckt = argv[1].p;

	strlcpy(name, OBJECT(ckt)->name, sizeof(name));
	strlcat(name, ".cir", sizeof(name));

	if (circuit_export_spice3(ckt, name) == -1)
		text_msg(MSG_ERROR, "%s: %s", OBJECT(ckt)->name, error_get());
}

static void
double_click(int argc, union evarg *argv)
{
	struct circuit *ckt = argv[1].p;
	int button = argv[2].i;
	int tx = argv[3].i;
	int ty = argv[4].i;
	int txoff = argv[5].i;
	int tyoff = argv[6].i;
	struct component *com;
	double x, y;

	vg_vcoords2(ckt->vg, tx, ty, txoff, tyoff, &x, &y);
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		struct vg_rect rext;

		if (!OBJECT_SUBCLASS(com, "component"))
			continue;

		vg_block_extent(ckt->vg, com->block, &rext);
		if (x > rext.x && y > rext.y &&
		    x < rext.x+rext.w && y < rext.y+rext.h) {
#if 0
			com->selected = !com->selected;
#endif
			/* XXX configurable */
			switch (button) {
			case 1:
				if (OBJECT(com)->ops->edit != NULL) {
					objmgr_open_data(com, 1);
				}
				break;
			case 2:
			case 3:
				com->selected = !com->selected;
				break;
			}
		}
	}
}

static void
select_sim(int argc, union evarg *argv)
{
	struct circuit *ckt = argv[1].p;
	struct sim_ops *sops = argv[2].p;
	struct window *pwin = argv[3].p;
	struct sim *sim;

	sim = circuit_set_sim(ckt, sops);
	if (sim->win != NULL)
		window_attach(pwin, sim->win);
}

static void
find_objs(struct tlist *tl, struct object *pob, int depth, void *ckt)
{
	struct object *cob;
	struct tlist_item *it;
	SDL_Surface *icon;

	it = tlist_insert(tl, object_icon(pob), "%s%s", pob->name,
	    (pob->flags & OBJECT_DATA_RESIDENT) ? " (resident)" : "");
	it->depth = depth;
	it->class = "object";
	it->p1 = pob;

	if (!TAILQ_EMPTY(&pob->children)) {
		it->flags |= TLIST_HAS_CHILDREN;
		if (pob == OBJECT(ckt))
			it->flags |= TLIST_VISIBLE_CHILDREN;
	}
	if ((it->flags & TLIST_HAS_CHILDREN) &&
	    tlist_visible_children(tl, it)) {
		TAILQ_FOREACH(cob, &pob->children, cobjs)
			find_objs(tl, cob, depth+1, ckt);
	}
}

static void
poll_objs(int argc, union evarg *argv)
{
	struct tlist *tl = argv[0].p;
	struct object *pob = argv[1].p;
	struct tlist_item *it;

	tlist_clear_items(tl);
	lock_linkage();
	find_objs(tl, pob, 0, pob);
	unlock_linkage();
	tlist_restore_selections(tl);
}

static void
create_view(int argc, union evarg *argv)
{
	extern struct tool_ops vg_scale_tool;
	struct mapview *omv = argv[1].p;
	struct window *pwin = argv[2].p;
	struct circuit *ckt = argv[3].p;
	struct map *map = omv->map;
	struct mapview *mv;
	struct window *win;

	win = window_new(0, NULL);
	window_set_caption(win, _("View of %s"), OBJECT(ckt)->name);
	{
		mv = mapview_new(win, map,
		    MAPVIEW_NO_BMPZOOM|MAPVIEW_NO_NODESEL,
		    NULL, NULL);
		mv->cam = map_add_camera(map, _("View"));

		mapview_prescale(mv, 8, 6);
		mapview_reg_draw_cb(mv, rasterize_circuit, ckt->vg);
		mapview_reg_tool(mv, &vg_scale_tool, ckt->vg);
		widget_focus(mv);
	}
	window_attach(pwin, win);
	window_show(win);
}

static void
poll_component_pins(int argc, union evarg *argv)
{
	struct tlist *tl = argv[0].p;
	struct circuit *ckt = argv[1].p;
	struct component *com;
	int i;
	
	tlist_clear_items(tl);
	
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (OBJECT_SUBCLASS(com, "component") && com->selected)
			break;
	}
	if (com == NULL)
		goto out;

	pthread_mutex_lock(&com->lock);
	for (i = 1; i <= com->npins; i++) {
		char text[TLIST_LABEL_MAX];
		struct pin *pin = &com->pin[i];

		snprintf(text, sizeof(text),
		    "%d (%s) (U=%.2f, \xce\x94U=%.2f) -> n%d\n",
		    pin->n, pin->name, pin->u, pin->du, pin->node);
		tlist_insert_item(tl, ICON(EDA_BRANCH_TO_COMPONENT_ICON),
		    text, pin);
	}
	pthread_mutex_unlock(&com->lock);
out:
	tlist_restore_selections(tl);
}

static void
poll_component_dipoles(int argc, union evarg *argv)
{
	struct tlist *tl = argv[0].p;
	struct circuit *ckt = argv[1].p;
	struct component *com;
	int i, j;
	
	tlist_clear_items(tl);
	
	OBJECT_FOREACH_CHILD(com, ckt, component) {
		if (OBJECT_SUBCLASS(com, "component") && com->selected)
			break;
	}
	if (com == NULL)
		goto out;

	pthread_mutex_lock(&com->lock);
	for (i = 0; i < com->ndipoles; i++) {
		char Rbuf[32], Cbuf[32], Lbuf[32];
		char text[TLIST_LABEL_MAX];
		struct dipole *dip = &com->dipoles[i];
		struct tlist_item *it;

		unit_format(dipole_resistance(dip), resistance_units, Rbuf,
		    sizeof(Rbuf));
		unit_format(dipole_capacitance(dip), capacitance_units, Cbuf,
		    sizeof(Cbuf));
		unit_format(dipole_inductance(dip), inductance_units, Lbuf,
		    sizeof(Lbuf));

		snprintf(text, sizeof(text),
		    "%s:(%s,%s) - %s, %s, %s",
		    OBJECT(com)->name, dip->p1->name, dip->p2->name,
		    Rbuf, Cbuf, Lbuf);
		it = tlist_insert_item(tl, ICON(EDA_BRANCH_TO_COMPONENT_ICON),
		    text, dip);
		it->depth = 0;

		for (j = 0; j < dip->nloops; j++) {
			struct cktloop *dloop = dip->loops[j];
			int dpol = dip->lpols[j];

			snprintf(text, sizeof(text), "%s:L%u (%s)",
			    OBJECT(dloop->origin)->name,
			    dloop->name,
			    dpol == 1 ? "+" : "-");
			it = tlist_insert_item(tl, ICON(EDA_MESH_ICON), text,
			    &dip->loops[j]);
			it->depth = 1;
		}
	}
	pthread_mutex_unlock(&com->lock);
out:
	tlist_restore_selections(tl);
}

struct window *
circuit_edit(void *p)
{
	struct circuit *ckt = p;
	struct window *win;
	struct toolbar *toolbar;
	struct statusbar *statbar;
	struct AGMenu *menu;
	struct AGMenuItem *pitem, *subitem;
	struct mapview *mv;
	struct hpane *pane;
	struct hpane_div *div;
	struct scrollbar *hbar, *vbar;

	win = window_new(0, NULL);
	window_set_caption(win, "%s", OBJECT(ckt)->name);

	toolbar = Malloc(sizeof(struct toolbar), M_OBJECT);
	toolbar_init(toolbar, TOOLBAR_VERT, 1, 0);
	statbar = Malloc(sizeof(struct statusbar), M_OBJECT);
	statusbar_init(statbar);
	mv = Malloc(sizeof(struct mapview), M_OBJECT);
	mapview_init(mv, ckt->vg->map, MAPVIEW_NO_BMPZOOM|MAPVIEW_NO_NODESEL|
	                               MAPVIEW_EDIT, toolbar, statbar);
	
	menu = menu_new(win);
	pitem = menu_add_item(menu, _("File"));
	{
		menu_action_kb(pitem, _("Revert"), OBJLOAD_ICON,
		    SDLK_r, KMOD_CTRL, revert_circuit, "%p", ckt);
		menu_action_kb(pitem, _("Save"), OBJSAVE_ICON,
		    SDLK_s, KMOD_CTRL, save_circuit, "%p", ckt);

		subitem = menu_action(pitem, _("Export circuit"), OBJSAVE_ICON,
		    NULL, NULL);
		menu_action(subitem, _("Export to SPICE3..."), EDA_SPICE_ICON,
		    export_to_spice, "%p", ckt);
	
		menu_separator(pitem);

		menu_action_kb(pitem, _("Close document"), CLOSE_ICON,
		    SDLK_w, KMOD_CTRL, window_generic_close, "%p", win);
	}
	pitem = menu_add_item(menu, _("Edit"));
	{
		/* TODO */
		menu_action(pitem, _("Undo"), -1, NULL, NULL);
		menu_action(pitem, _("Redo"), -1, NULL, NULL);

		menu_separator(pitem);

		menu_action(pitem, _("Layout preferences..."), SETTINGS_ICON,
		    layout_settings, "%p,%p", win, ckt);
	}

	pitem = menu_add_item(menu, _("Drawing"));
	vg_reg_menu(menu, pitem, ckt->vg, mv);

	pitem = menu_add_item(menu, _("View"));
	{
		menu_action(pitem, _("Show interconnects..."), EDA_MESH_ICON,
		    show_interconnects, "%p,%p", win, ckt);

		menu_separator(pitem);

		menu_int_flags(pitem, _("Show origin"), VGORIGIN_ICON,
		    &ckt->vg->flags, VG_VISORIGIN, 0);
		menu_int_flags(pitem, _("Show grid"), GRID_ICON,
		    &ckt->vg->flags, VG_VISGRID, 0);
		menu_int_flags(pitem, _("Show extents"), VGBLOCK_ICON,
		    &ckt->vg->flags, VG_VISBBOXES, 0);
		
		menu_separator(pitem);
		
		menu_action(pitem, _("Create view..."), NEW_VIEW_ICON,
		    create_view, "%p,%p,%p", mv, win, ckt);
	}

	pitem = menu_add_item(menu, _("Simulation"));
	{
		extern const struct sim_ops *sim_ops[];
		const struct sim_ops **ops;

		for (ops = &sim_ops[0]; *ops != NULL; ops++) {
			menu_tool(pitem, toolbar, _((*ops)->name),
			    (*ops)->icon, 0, 0,
			    select_sim, "%p,%p,%p", ckt, *ops, win);
		}
	}
#if 0
	pitem = menu_add_item(menu, _("Component"));
	component_reg_menu(menu, pitem, ckt, mv);
#endif

	pane = hpane_new(win, HPANE_WFILL|HPANE_HFILL);
	div = hpane_add_div(pane,
	    BOX_VERT, BOX_HFILL,
	    BOX_HORIZ, BOX_WFILL|BOX_HFILL);
	{
		struct notebook *nb;
		struct notebook_tab *ntab;
		struct tlist *tl;
		struct box *box;

		nb = notebook_new(div->box1, NOTEBOOK_HFILL|NOTEBOOK_WFILL);
		ntab = notebook_add_tab(nb, _("Models"), BOX_VERT);
		{
			char tname[OBJECT_TYPE_MAX];
			extern const struct eda_type eda_models[];
			const struct eda_type *ty;
			struct button *btn;
			int i;

			tl = tlist_new(ntab, TLIST_POLL|TLIST_TREE);
			WIDGET(tl)->flags &= ~(WIDGET_FOCUSABLE);
			event_new(tl, "tlist-dblclick", component_insert,
			    "%p,%p,%p", mv, tl, ckt);

			btn = button_new(ntab, _("Insert component"));
			WIDGET(btn)->flags |= WIDGET_WFILL;
			event_new(btn, "button-pushed", component_insert,
			    "%p,%p,%p", mv, tl, ckt);

			for (ty = &eda_models[0]; ty->name != NULL; ty++) {
				for (i = 0; i < ntypesw; i++) {
					strlcpy(tname, "component.",
					    sizeof(tname));
					strlcat(tname, ty->name, sizeof(tname));
					if (strcmp(typesw[i].type, tname) == 0)
						break;
				}
				if (i < ntypesw) {
					struct object_type *ctype = &typesw[i];
					struct component_ops *cops =
					    (struct component_ops *)ctype->ops;

					tlist_insert_item(tl,
					    ICON(EDA_COMPONENT_ICON),
					    _(cops->name), ctype);
				}
			}
		}

		ntab = notebook_add_tab(nb, _("Objects"), BOX_VERT);
		{
			tl = tlist_new(ntab, TLIST_POLL|TLIST_TREE);
			event_new(tl, "tlist-poll", poll_objs, "%p", ckt);
			mv->objs_tl = tl;
			WIDGET(tl)->flags &= ~(WIDGET_FOCUSABLE);
		}

		ntab = notebook_add_tab(nb, _("Component"), BOX_VERT);
		{
#if 0
			struct fspinbutton *fsb;

			label_static(ntab, _("Temperature:"));

			fsb = fspinbutton_new(win, "degC", _("Reference: "));
			widget_bind(fsb, "value", WIDGET_FLOAT, &com->Tnom);
			fspinbutton_set_min(fsb, 0.0);
	
			fsb = fspinbutton_new(win, "degC", _("Instance: "));
			widget_bind(fsb, "value", WIDGET_FLOAT, &com->Tspec);
			fspinbutton_set_min(fsb, 0.0);

			separator_new(ntab, SEPARATOR_HORIZ);
#endif
			label_new(ntab, LABEL_STATIC, _("Pinout:"));
			{
				tl = tlist_new(ntab, TLIST_POLL);
				WIDGET(tl)->flags &= ~(WIDGET_HFILL);
				event_new(tl, "tlist-poll",
				    poll_component_pins, "%p", ckt);
			}
			label_new(ntab, LABEL_STATIC, _("Dipoles:"));
			{
				tl = tlist_new(ntab, TLIST_POLL);
				event_new(tl, "tlist-poll",
				    poll_component_dipoles, "%p", ckt);
			}
		}

		vbar = scrollbar_new(div->box2, SCROLLBAR_VERT);
		box = box_new(div->box2, BOX_VERT, BOX_WFILL|BOX_HFILL);
		{
			object_attach(box, mv);
			widget_focus(mv);
			hbar = scrollbar_new(box, SCROLLBAR_HORIZ);
		}
		object_attach(div->box2, toolbar);
	}

	{
		extern struct tool_ops component_ins_tool;
		extern struct tool_ops component_sel_tool;
		extern struct tool_ops conductor_tool;
		extern struct tool_ops vg_scale_tool;
		extern struct tool_ops vg_origin_tool;
		extern struct tool_ops vg_line_tool;
		extern struct tool_ops vg_text_tool;
		struct tool *seltool;

		mapview_prescale(mv, 10, 8);
		mapview_reg_draw_cb(mv, rasterize_circuit, ckt->vg);
		mapview_set_scrollbars(mv, hbar, vbar);
		
		mapview_reg_tool(mv, &component_ins_tool, ckt);
		mapview_reg_tool(mv, &conductor_tool, ckt);
		mapview_reg_tool(mv, &vg_scale_tool, ckt->vg);
		mapview_reg_tool(mv, &vg_origin_tool, ckt->vg);
		mapview_reg_tool(mv, &vg_line_tool, ckt->vg);
		mapview_reg_tool(mv, &vg_text_tool, ckt->vg);
		
		seltool = mapview_reg_tool(mv, &component_sel_tool, ckt);
		mapview_set_default_tool(mv, seltool);

		event_new(mv, "mapview-dblclick", double_click, "%p", ckt);
	}

	object_attach(win, statbar);
	window_scale(win, -1, -1);
	window_set_geometry(win,
	    view->w/6, view->h/6,
	    2*view->w/3, 2*view->h/3);
	
	return (win);
}
#endif /* EDITION */
