/*	$Csoft: circuit.c,v 1.11 2005/09/29 00:22:33 vedge Exp $	*/

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

#include "aeda.h"
#include "circuit.h"
#include "spice.h"

const AG_Version circuit_ver = {
	"agar-eda circuit",
	0, 0
};

const AG_ObjectOps circuit_ops = {
	circuit_init,
	circuit_reinit,
	circuit_destroy,
	circuit_load,
	circuit_save,
	circuit_edit
};

static void init_node(struct cktnode *, u_int);
static void init_ground(struct circuit *);

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

	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!AGOBJECT_SUBCLASS(com, "component"))
			continue;

		AG_PostEvent(ckt, com, "circuit-shown", NULL);
		AG_ObjectPageIn(com, AG_OBJECT_DATA);
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

	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!AGOBJECT_SUBCLASS(com, "component"))
			continue;

		AG_PostEvent(ckt, com, "circuit-hidden", NULL);
		AG_ObjectPageOut(com, AG_OBJECT_DATA);
	}
}

/* Effect a change in the circuit topology.  */
void
circuit_modified(struct circuit *ckt)
{
	struct component *com;
	struct vsource *vs;
	struct cktloop *loop;
	u_int i;

	/* Regenerate loop and dipole information. */
	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!AGOBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING)
			continue;

		for (i = 0; i < com->ndipoles; i++)
			com->dipoles[i].nloops = 0;
	}
	AGOBJECT_FOREACH_CHILD(vs, ckt, vsource) {
		if (!AGOBJECT_SUBCLASS(vs, "component.vsource") ||
		    COM(vs)->flags & COMPONENT_FLOATING ||
		    PIN(vs,1)->node == NULL ||
		    PIN(vs,2)->node == NULL) {
			continue;
		}
		vsource_free_loops(vs);
		vsource_find_loops(vs);
	}
#if 0
	AGOBJECT_FOREACH_CHILD(com, ckt, component)
		AG_PostEvent(ckt, com, "circuit-modified", NULL);
#endif
	if (ckt->loops == NULL) {
		ckt->loops = Malloc(sizeof(struct cktloop *), M_EDA);
	}
	ckt->l = 0;
	AGOBJECT_FOREACH_CHILD(vs, ckt, vsource) {
		if (!AGOBJECT_SUBCLASS(vs, "component.vsource") ||
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
	VG_Style *vgs;
	VG *vg;

	AG_ObjectInit(ckt, "circuit", name, &circuit_ops);
	ckt->sim = NULL;
	ckt->dis_nodes = 1;
	ckt->dis_node_names = 1;
	pthread_mutex_init(&ckt->lock, &agRecursiveMutexAttr);

	ckt->loops = NULL;
	ckt->vsrcs = NULL;
	ckt->m = 0;
	
	ckt->nodes = Malloc(sizeof(struct cktnode *), M_EDA);
	ckt->n = 0;
	init_ground(ckt);

	vg = ckt->vg = VG_New(ckt, VG_VISGRID);
	strlcpy(vg->layers[0].name, _("Schematic"), sizeof(vg->layers[0].name));
	VG_Scale(vg, 11.0, 8.5, 2.0);
	VG_DefaultScale(vg, 2.0);
	VG_SetGridGap(vg, 0.25);
	vg->origin[0].x = 5.5;
	vg->origin[0].y = 4.25;
	VG_OriginRadius(vg, 2, 0.09375);
	VG_OriginColor(vg, 2, 255, 255, 165);

	vgs = VG_CreateStyle(vg, VG_TEXT_STYLE, "component-name");
	vgs->vg_text_st.size = 10;
	
	vgs = VG_CreateStyle(vg, VG_TEXT_STYLE, "node-name");
	vgs->vg_text_st.size = 8;
	
	AG_SetEvent(ckt, "edit-open", circuit_opened, NULL);
	AG_SetEvent(ckt, "edit-close", circuit_closed, NULL);
}

void
circuit_reinit(void *p)
{
	struct circuit *ckt = p;
	struct component *com;
	u_int i;
	
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
	
	for (i = 0; i <= ckt->n; i++) {
		circuit_destroy_node(ckt->nodes[i]);
		Free(ckt->nodes[i], M_EDA);
	}
	ckt->nodes = Realloc(ckt->nodes, sizeof(struct cktnode *));
	ckt->n = 0;
	init_ground(ckt);

	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!AGOBJECT_SUBCLASS(com, "component") ||
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
	VG_Reinit(ckt->vg);
}

int
circuit_load(void *p, AG_Netbuf *buf)
{
	struct circuit *ckt = p;
	Uint32 ncoms;
	u_int i, j, nnodes;

	if (AG_ReadVersion(buf, &circuit_ver, NULL) != 0) {
		return (-1);
	}
	AG_CopyString(ckt->descr, buf, sizeof(ckt->descr));
	AG_ReadUint32(buf);					/* Pad */

	/* Load the circuit nodes (including ground). */
	nnodes = (u_int)AG_ReadUint32(buf);
	for (i = 0; i <= nnodes; i++) {
		int nbranches, flags;
		u_int name;

		flags = (int)AG_ReadUint32(buf);
		nbranches = (int)AG_ReadUint32(buf);
		if (i == 0) {
			name = 0;
			circuit_destroy_node(ckt->nodes[0]);
			Free(ckt->nodes[0], M_EDA);
			init_ground(ckt);
		} else {
			name = circuit_add_node(ckt, flags & ~(CKTNODE_EXAM));
		}
		for (j = 0; j < nbranches; j++) {
			char ppath[AG_OBJECT_PATH_MAX];
			struct component *pcom;
			struct cktbranch *br;
			int ppin;

			AG_CopyString(ppath, buf, sizeof(ppath));
			AG_ReadUint32(buf);			/* Pad */
			ppin = (int)AG_ReadUint32(buf);
			if ((pcom = AG_ObjectFind(ppath)) == NULL) {
				AG_SetError("%s", AG_GetError());
				return (-1);
			}
			if (ppin > pcom->npins) {
				AG_SetError("%s: pin #%d > %d", ppath, ppin,
				    pcom->npins);
				return (-1);
			}
			br = circuit_add_branch(ckt, name, &pcom->pin[ppin]);
			pcom->pin[ppin].node = name;
			pcom->pin[ppin].branch = br;
		}
	}

	/* Load the schematics. */
	if (VG_Load(ckt->vg, buf) == -1)
		return (-1);

	/* Load the component pin assignments. */
	ncoms = AG_ReadUint32(buf);
	for (i = 0; i < ncoms; i++) {
		char name[AG_OBJECT_NAME_MAX];
		char path[AG_OBJECT_PATH_MAX];
		int j, npins;
		struct component *com;

		/* Lookup the component. */
		AG_CopyString(name, buf, sizeof(name));
		AGOBJECT_FOREACH_CHILD(com, ckt, component) {
			if (AGOBJECT_SUBCLASS(com, "component") &&
			    strcmp(AGOBJECT(com)->name, name) == 0)
				break;
		}
		if (com == NULL)
			fatal("unexisting component: `%s'", name);
		
		/* Reassign the block pointer since the vg was reloaded. */
		AG_ObjectCopyName(com, path, sizeof(path));
		if ((com->block = VG_GetBlock(ckt->vg, path)) == NULL)
			fatal("unexisting block: `%s'", path);

		/* Load the pinout information. */
		com->npins = (int)AG_ReadUint32(buf);
		for (j = 1; j <= com->npins; j++) {
			struct pin *pin = &com->pin[j];
			VG_Block *block_save;
			struct cktbranch *br;
			struct cktnode *node;

			pin->n = (int)AG_ReadUint32(buf);
			AG_CopyString(pin->name, buf, sizeof(pin->name));
			pin->x = AG_ReadDouble(buf);
			pin->y = AG_ReadDouble(buf);
			pin->node = (int)AG_ReadUint32(buf);
			node = ckt->nodes[pin->node];

			TAILQ_FOREACH(br, &node->branches, branches) {
				if (br->pin->com == com &&
				    br->pin->n == pin->n)
					break;
			}
			if (br == NULL) {
				fatal("no such branch: %s:%d",
				    AGOBJECT(com)->name, pin->n);
			}
			pin->branch = br;
			pin->selected = 0;
		}
	}
	circuit_modified(ckt);
	return (0);
}

int
circuit_save(void *p, AG_Netbuf *buf)
{
	char path[AG_OBJECT_PATH_MAX];
	struct circuit *ckt = p;
	struct cktnode *node;
	struct cktbranch *br;
	struct component *com;
	off_t ncoms_offs;
	Uint32 ncoms = 0;
	u_int i;

	AG_WriteVersion(buf, &circuit_ver);
	AG_WriteString(buf, ckt->descr);
	AG_WriteUint32(buf, 0);					/* Pad */
	AG_WriteUint32(buf, ckt->n);
	for (i = 0; i <= ckt->n; i++) {
		struct cktnode *node = ckt->nodes[i];
		off_t nbranches_offs;
		Uint32 nbranches = 0;

		AG_WriteUint32(buf, (Uint32)node->flags);
		nbranches_offs = AG_NetbufTell(buf);
		AG_WriteUint32(buf, 0);
		TAILQ_FOREACH(br, &node->branches, branches) {
			if (br->pin == NULL || br->pin->com == NULL ||
			    br->pin->com->flags & COMPONENT_FLOATING) {
				continue;
			}
			AG_ObjectCopyName(br->pin->com, path, sizeof(path));
			AG_WriteString(buf, path);
			AG_WriteUint32(buf, 0);			/* Pad */
			AG_WriteUint32(buf, (Uint32)br->pin->n);
			nbranches++;
		}
		AG_PwriteUint32(buf, nbranches, nbranches_offs);
	}
	
	/* Save the schematics. */
	VG_Save(ckt->vg, buf);
	
	/* Save the component information. */
	ncoms_offs = AG_NetbufTell(buf);
	AG_WriteUint32(buf, 0);
	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!AGOBJECT_SUBCLASS(com, "component") ||
		    com->flags & COMPONENT_FLOATING) {
			continue;
		}
		AG_WriteString(buf, AGOBJECT(com)->name);
		AG_WriteUint32(buf, (Uint32)com->npins);
		for (i = 1; i <= com->npins; i++) {
			struct pin *pin = &com->pin[i];

			AG_WriteUint32(buf, (Uint32)pin->n);
			AG_WriteString(buf, pin->name);
			AG_WriteDouble(buf, pin->x);
			AG_WriteDouble(buf, pin->y);
#ifdef DEBUG
			if (pin->node == -1)
				fatal("%s: bad pin %d", AGOBJECT(com)->name, i);
#endif
			AG_WriteUint32(buf, (Uint32)pin->node);
		}
		ncoms++;
	}
	AG_PwriteUint32(buf, ncoms, ncoms_offs);
	return (0);
}

static void
init_node(struct cktnode *node, u_int flags)
{
	node->flags = flags;
	node->nbranches = 0;
	TAILQ_INIT(&node->branches);
}

static void
init_ground(struct circuit *ckt)
{
	ckt->nodes[0] = Malloc(sizeof(struct cktnode), M_EDA);
	init_node(ckt->nodes[0], CKTNODE_REFERENCE);
}

int
circuit_add_node(struct circuit *ckt, u_int flags)
{
	struct cktnode *node;
	u_int n = ++ckt->n;

	ckt->nodes = Realloc(ckt->nodes, (n+1)*sizeof(struct cktnode *));
	ckt->nodes[n] = Malloc(sizeof(struct cktnode), M_EDA);
	init_node(ckt->nodes[n], flags);
	return (n);
}

/* Evaluate whether node n is at ground potential. */
int
cktnode_grounded(struct circuit *ckt, u_int n)
{
	/* TODO check for shunts */
	return (n == 0 ? 1 : 0);
}

/*
 * Evaluate whether the given node is connected to the given voltage source.
 * If that is the case, return the polarity of the node with respect to
 * the source.
 */
int
cktnode_vsource(struct circuit *ckt, u_int n, u_int m, int *sign)
{
	struct cktnode *node = ckt->nodes[n];
	struct cktbranch *br;
	struct vsource *vs;
	u_int i;

	TAILQ_FOREACH(br, &node->branches, branches) {
		if (br->pin == NULL ||
		   (vs = VSOURCE(br->pin->com)) == NULL) {
			continue;
		}
		if (COM(vs)->flags & COMPONENT_FLOATING ||
		   !AGOBJECT_SUBCLASS(vs, "component.vsource") ||
		   vs != ckt->vsrcs[m-1]) {
			continue;
		}
		if (br->pin->n == 1) {
			*sign = +1;
		} else {
			*sign = -1;
		}
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
circuit_del_node(struct circuit *ckt, u_int n)
{
	struct cktnode *node;
	struct cktbranch *br;
	u_int i;

#ifdef DEBUG
	if (n == 0 || n > ckt->n)
		fatal("bad node: n%u\n", n);
#endif
	node = ckt->nodes[n];
	circuit_destroy_node(node);
	Free(node, M_EDA);

	if (n != ckt->n) {
		for (i = n; i <= ckt->n; i++) {
			TAILQ_FOREACH(br, &ckt->nodes[i]->branches, branches) {
				if (br->pin != NULL && br->pin->com != NULL)
					br->pin->node = i-1;
			}
		}
		memmove(&ckt->nodes[n], &ckt->nodes[n+1],
		    (ckt->n - n) * sizeof(struct cktnode *));
	}
	ckt->n--;
}

struct cktbranch *
circuit_add_branch(struct circuit *ckt, u_int n, struct pin *pin)
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
circuit_get_branch(struct circuit *ckt, u_int n, struct pin *pin)
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
circuit_del_branch(struct circuit *ckt, u_int n, struct cktbranch *br)
{
	struct cktnode *node = ckt->nodes[n];

	TAILQ_REMOVE(&node->branches, br, branches);
	Free(br, M_EDA);
#ifdef DEBUG
	if ((node->nbranches - 1) < 0) { fatal("--nbranches < 0"); }
#endif
	node->nbranches--;
}

void
circuit_free_components(struct circuit *ckt)
{
	struct component *com, *ncom;

	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (!AGOBJECT_SUBCLASS(com, "component")) {
			continue;
		}
		AG_ObjectDetach(com);
		AG_ObjectDestroy(com);
		Free(com, M_OBJECT);
	}
}

void
circuit_destroy(void *p)
{
	struct circuit *ckt = p;
	
	VG_Destroy(ckt->vg);
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
		AG_WindowSetCaption(sim->win, "%s: %s", AGOBJECT(ckt)->name,
		    _(sim->ops->name));
		AG_WindowSetPosition(sim->win, AG_WINDOW_LOWER_LEFT, 0);
		AG_WindowShow(sim->win);
	}
	return (sim);
}

#ifdef EDITION

static void
rasterize_circuit(AG_Mapview *mv, void *p)
{
	VG *vg = p;

	if (vg->redraw) {
		vg->redraw = 0;
	}
	vg->redraw++;
	VG_Rasterize(vg);
}

static void
poll_loops(int argc, union evarg *argv)
{
	AG_Tlist *tl = argv[0].p;
	struct circuit *ckt = argv[1].p;
	int i;

	AG_TlistClear(tl);
	for (i = 0; i < ckt->l; i++) {
		char voltage[32];
		struct cktloop *loop = ckt->loops[i];
		struct vsource *vs = (struct vsource *)loop->origin;
		AG_TlistItem *it;

		AG_UnitFormat(vs->voltage, agEMFUnits, voltage, sizeof(voltage));
		it = AG_TlistAdd(tl, NULL, "%s:L%u (%s)", AGOBJECT(vs)->name,
		    loop->name, voltage);
		it->p1 = loop;
	}
	AG_TlistRestore(tl);
}

static void
poll_nodes(int argc, union evarg *argv)
{
	AG_Tlist *tl = argv[0].p;
	struct circuit *ckt = argv[1].p;
	u_int i;

	AG_TlistClear(tl);
	for (i = 0; i <= ckt->n; i++) {
		struct cktnode *node = ckt->nodes[i];
		struct cktbranch *br;
		AG_TlistItem *it, *it2;

		it = AG_TlistAdd(tl, NULL, "n%u (0x%x, %d branches)", i,
		    node->flags, node->nbranches);
		it->p1 = node;
		it->depth = 0;
		TAILQ_FOREACH(br, &node->branches, branches) {
			if (br->pin == NULL) {
				it = AG_TlistAdd(tl, NULL, "(null pin)");
			} else {
				it = AG_TlistAdd(tl, NULL, "%s:%s(%d)",
				    (br->pin != NULL && br->pin->com != NULL) ?
				    AGOBJECT(br->pin->com)->name : "(null)",
				    br->pin->name, br->pin->n);
			}
			it->p1 = br;
			it->depth = 1;
		}
	}
	AG_TlistRestore(tl);
}

static void
poll_sources(int argc, union evarg *argv)
{
	AG_Tlist *tl = argv[0].p;
	struct circuit *ckt = argv[1].p;
	int i;

	AG_TlistClear(tl);
	for (i = 0; i < ckt->m; i++) {
		struct vsource *vs = ckt->vsrcs[i];
		AG_TlistItem *it;

		it = AG_TlistAdd(tl, NULL, "%s", AGOBJECT(vs)->name);
		it->p1 = vs;
	}
	AG_TlistRestore(tl);
}

static void
revert_circuit(int argc, union evarg *argv)
{
	struct circuit *ckt = argv[1].p;

	if (AG_ObjectLoad(ckt) == 0) {
		AG_TextTmsg(AG_MSG_INFO, 1000,
		    _("Circuit `%s' reverted successfully."),
		    AGOBJECT(ckt)->name);
	} else {
		AG_TextMsg(AG_MSG_ERROR, "%s: %s", AGOBJECT(ckt)->name,
		    AG_GetError());
	}
}

static void
save_circuit(int argc, union evarg *argv)
{
	struct circuit *ckt = argv[1].p;

	if (AG_ObjectSave(agWorld) == 0 &&
	    AG_ObjectSave(ckt) == 0) {
		AG_TextTmsg(AG_MSG_INFO, 1250,
		    _("Circuit `%s' saved successfully."),
		    AGOBJECT(ckt)->name);
	} else {
		AG_TextMsg(AG_MSG_ERROR, "%s: %s", AGOBJECT(ckt)->name,
		    AG_GetError());
	}
}

static void
show_interconnects(int argc, union evarg *argv)
{
	AG_Window *pwin = argv[1].p;
	struct circuit *ckt = argv[2].p;
	AG_Window *win;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;
	AG_Tlist *tl;
	
	if ((win = AG_WindowNewNamed(0, "%s-interconnects",
	    AGOBJECT(ckt)->name)) == NULL) {
		return;
	}
	AG_WindowSetCaption(win, _("%s: Connections"), AGOBJECT(ckt)->name);
	AG_WindowSetPosition(win, AG_WINDOW_UPPER_RIGHT, 0);
	
	nb = AG_NotebookNew(win, AG_NOTEBOOK_HFILL|AG_NOTEBOOK_WFILL);
	ntab = AG_NotebookAddTab(nb, _("Nodes"), AG_BOX_VERT);
	{
		tl = AG_TlistNew(ntab, AG_TLIST_POLL|AG_TLIST_TREE);
		AG_SetEvent(tl, "tlist-poll", poll_nodes, "%p", ckt);
	}
	
	ntab = AG_NotebookAddTab(nb, _("Loops"), AG_BOX_VERT);
	{
		tl = AG_TlistNew(ntab, AG_TLIST_POLL);
		AG_SetEvent(tl, "tlist-poll", poll_loops, "%p", ckt);
	}
	
	ntab = AG_NotebookAddTab(nb, _("Sources"), AG_BOX_VERT);
	{
		tl = AG_TlistNew(ntab, AG_TLIST_POLL);
		AG_SetEvent(tl, "tlist-poll", poll_sources, "%p", ckt);
	}

	AG_WindowAttach(pwin, win);
	AG_WindowShow(win);
}

static void
layout_settings(int argc, union evarg *argv)
{
	char path[AG_OBJECT_PATH_MAX];
	AG_Window *pwin = argv[1].p;
	struct circuit *ckt = argv[2].p;
	VG *vg = ckt->vg;
	AG_Window *win;
	AG_MFSpinbutton *mfsu;
	AG_FSpinbutton *fsu;
	AG_Textbox *tb;
	AG_Checkbox *cb;
	
	AG_ObjectCopyName(ckt, path, sizeof(path));
	if ((win = AG_WindowNewNamed(0, "settings-%s", path)) == NULL) {
		return;
	}
	AG_WindowSetCaption(win, _("Circuit layout: %s"), AGOBJECT(ckt)->name);
	AG_WindowSetPosition(win, AG_WINDOW_UPPER_RIGHT, 0);

	AG_LabelNew(win, AG_LABEL_POLLED, _("Loops: %u"), &ckt->l);
	AG_LabelNew(win, AG_LABEL_POLLED, _("Voltage sources: %u"), &ckt->m);
	AG_LabelNew(win, AG_LABEL_POLLED, _("Nodes: %u"), &ckt->n);

	tb = AG_TextboxNew(win, _("Description: "));
	AG_WidgetBind(tb, "string", AG_WIDGET_STRING, &ckt->descr,
	    sizeof(ckt->descr));

	mfsu = AG_MFSpinbuttonNew(win, NULL, "x", _("Bounding box: "));
	AGWIDGET(mfsu)->flags |= AG_WIDGET_WFILL;
	AG_WidgetBind(mfsu, "xvalue", AG_WIDGET_DOUBLE, &vg->w);
	AG_WidgetBind(mfsu, "yvalue", AG_WIDGET_DOUBLE, &vg->h);
	AG_MFSpinbuttonSetMin(mfsu, 1.0);
	AG_MFSpinbuttonSetIncrement(mfsu, 0.1);
	AG_SetEvent(mfsu, "mfspinbutton-changed", VG_GeoChangedEv, "%p", vg);

	fsu = AG_FSpinbuttonNew(win, NULL, _("Grid interval: "));
	AGWIDGET(fsu)->flags |= AG_WIDGET_WFILL;
	AG_WidgetBind(fsu, "value", AG_WIDGET_DOUBLE, &vg->grid_gap);
	AG_FSpinbuttonSetMin(fsu, 0.0625);
	AG_FSpinbuttonSetIncrement(fsu, 0.0625);
	AG_SetEvent(fsu, "fspinbutton-changed", VG_ChangedEv, "%p", vg);
	
	fsu = AG_FSpinbuttonNew(win, NULL, _("Scaling factor: "));
	AGWIDGET(fsu)->flags |= AG_WIDGET_WFILL;
	AG_WidgetBind(fsu, "value", AG_WIDGET_DOUBLE, &vg->scale);
	AG_FSpinbuttonSetMin(fsu, 0.1);
	AG_FSpinbuttonSetIncrement(fsu, 0.1);
	AG_SetEvent(fsu, "fspinbutton-changed", VG_GeoChangedEv, "%p", vg);

	cb = AG_CheckboxNew(win, _("Display node indicators"));
	AG_WidgetBind(cb, "state", AG_WIDGET_INT, &ckt->dis_nodes);

	cb = AG_CheckboxNew(win, _("Display node names"));
	AG_WidgetBind(cb, "state", AG_WIDGET_INT, &ckt->dis_node_names);
	
#if 0
	AG_LabelNew(win, AG_LABEL_STATIC, _("Nodes:"));
	tl = AG_TlistNew(win, AG_TLIST_POLL|AG_TLIST_TREE);
	AGWIDGET(tl)->flags &= ~(AG_WIDGET_HFILL);
	AG_SetEvent(tl, "tlist-poll", poll_nodes, "%p", ckt);
#endif	

	AG_WindowAttach(pwin, win);
	AG_WindowShow(win);
}

static void
export_to_spice(int argc, union evarg *argv)
{
	char name[FILENAME_MAX];
	struct circuit *ckt = argv[1].p;

	strlcpy(name, AGOBJECT(ckt)->name, sizeof(name));
	strlcat(name, ".cir", sizeof(name));

	if (circuit_export_spice3(ckt, name) == -1)
		AG_TextMsg(AG_MSG_ERROR, "%s: %s", AGOBJECT(ckt)->name,
		    AG_GetError());
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

	VG_Vcoords2(ckt->vg, tx, ty, txoff, tyoff, &x, &y);
	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		VG_Rect rext;

		if (!AGOBJECT_SUBCLASS(com, "component"))
			continue;

		VG_BlockExtent(ckt->vg, com->block, &rext);
		if (x > rext.x && y > rext.y &&
		    x < rext.x+rext.w && y < rext.y+rext.h) {
#if 0
			com->selected = !com->selected;
#endif
			/* XXX configurable */
			switch (button) {
			case 1:
				if (AGOBJECT(com)->ops->edit != NULL) {
					AG_ObjMgrOpenData(com, 1);
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
	AG_Window *pwin = argv[3].p;
	struct sim *sim;

	sim = circuit_set_sim(ckt, sops);
	if (sim->win != NULL)
		AG_WindowAttach(pwin, sim->win);
}

static void
find_objs(AG_Tlist *tl, AG_Object *pob, int depth, void *ckt)
{
	AG_Object *cob;
	AG_TlistItem *it;
	SDL_Surface *icon;

	it = AG_TlistAdd(tl, AG_ObjectIcon(pob), "%s%s", pob->name,
	    (pob->flags & AG_OBJECT_DATA_RESIDENT) ? " (resident)" : "");
	it->depth = depth;
	it->class = "object";
	it->p1 = pob;

	if (!TAILQ_EMPTY(&pob->children)) {
		it->flags |= AG_TLIST_HAS_CHILDREN;
		if (pob == AGOBJECT(ckt))
			it->flags |= AG_TLIST_VISIBLE_CHILDREN;
	}
	if ((it->flags & AG_TLIST_HAS_CHILDREN) &&
	    AG_TlistVisibleChildren(tl, it)) {
		TAILQ_FOREACH(cob, &pob->children, cobjs)
			find_objs(tl, cob, depth+1, ckt);
	}
}

static void
poll_objs(int argc, union evarg *argv)
{
	AG_Tlist *tl = argv[0].p;
	AG_Object *pob = argv[1].p;
	AG_TlistItem *it;

	AG_TlistClear(tl);
	AG_LockLinkage();
	find_objs(tl, pob, 0, pob);
	AG_UnlockLinkage();
	AG_TlistRestore(tl);
}

static void
create_view(int argc, union evarg *argv)
{
	extern AG_MaptoolOps vgScaleTool;
	AG_Mapview *omv = argv[1].p;
	AG_Window *pwin = argv[2].p;
	struct circuit *ckt = argv[3].p;
	AG_Map *map = omv->map;
	AG_Mapview *mv;
	AG_Window *win;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("View of %s"), AGOBJECT(ckt)->name);
	{
		mv = AG_MapviewNew(win, map,
		    AG_MAPVIEW_NO_BMPSCALE|AG_MAPVIEW_NO_NODESEL,
		    NULL, NULL);
		mv->cam = AG_MapAddCamera(map, _("View"));

		AG_MapviewPrescale(mv, 8, 6);
		AG_MapviewRegDrawCb(mv, rasterize_circuit, ckt->vg);
		AG_MapviewRegTool(mv, &vgScaleTool, ckt->vg);
		AG_WidgetFocus(mv);
	}
	AG_WindowAttach(pwin, win);
	AG_WindowShow(win);
}

static void
poll_component_pins(int argc, union evarg *argv)
{
	AG_Tlist *tl = argv[0].p;
	struct circuit *ckt = argv[1].p;
	struct component *com;
	int i;
	
	AG_TlistClear(tl);
	
	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (AGOBJECT_SUBCLASS(com, "component") && com->selected)
			break;
	}
	if (com == NULL)
		goto out;

	pthread_mutex_lock(&com->lock);
	for (i = 1; i <= com->npins; i++) {
		char text[AG_TLIST_LABEL_MAX];
		struct pin *pin = &com->pin[i];

		snprintf(text, sizeof(text),
		    "%d (%s) (U=%.2f, \xce\x94U=%.2f) -> n%d\n",
		    pin->n, pin->name, pin->u, pin->du, pin->node);
		AG_TlistAddPtr(tl, AGICON(EDA_BRANCH_TO_COMPONENT_ICON),
		    text, pin);
	}
	pthread_mutex_unlock(&com->lock);
out:
	AG_TlistRestore(tl);
}

static void
poll_component_dipoles(int argc, union evarg *argv)
{
	AG_Tlist *tl = argv[0].p;
	struct circuit *ckt = argv[1].p;
	struct component *com;
	int i, j;
	
	AG_TlistClear(tl);
	
	AGOBJECT_FOREACH_CHILD(com, ckt, component) {
		if (AGOBJECT_SUBCLASS(com, "component") && com->selected)
			break;
	}
	if (com == NULL)
		goto out;

	pthread_mutex_lock(&com->lock);
	for (i = 0; i < com->ndipoles; i++) {
		char Rbuf[32], Cbuf[32], Lbuf[32];
		char text[AG_TLIST_LABEL_MAX];
		struct dipole *dip = &com->dipoles[i];
		AG_TlistItem *it;

		AG_UnitFormat(dipole_resistance(dip), agResistanceUnits, Rbuf,
		    sizeof(Rbuf));
		AG_UnitFormat(dipole_capacitance(dip), agCapacitanceUnits, Cbuf,
		    sizeof(Cbuf));
		AG_UnitFormat(dipole_inductance(dip), agInductanceUnits, Lbuf,
		    sizeof(Lbuf));

		snprintf(text, sizeof(text),
		    "%s:(%s,%s) - %s, %s, %s",
		    AGOBJECT(com)->name, dip->p1->name, dip->p2->name,
		    Rbuf, Cbuf, Lbuf);
		it = AG_TlistAddPtr(tl, AGICON(EDA_BRANCH_TO_COMPONENT_ICON),
		    text, dip);
		it->depth = 0;

		for (j = 0; j < dip->nloops; j++) {
			struct cktloop *dloop = dip->loops[j];
			int dpol = dip->lpols[j];

			snprintf(text, sizeof(text), "%s:L%u (%s)",
			    AGOBJECT(dloop->origin)->name,
			    dloop->name,
			    dpol == 1 ? "+" : "-");
			it = AG_TlistAddPtr(tl, AGICON(EDA_MESH_ICON), text,
			    &dip->loops[j]);
			it->depth = 1;
		}
	}
	pthread_mutex_unlock(&com->lock);
out:
	AG_TlistRestore(tl);
}

void *
circuit_edit(void *p)
{
	struct circuit *ckt = p;
	AG_Window *win;
	AG_Toolbar *toolbar;
	AG_Statusbar *statbar;
	AG_Menu *menu;
	AG_MenuItem *pitem, *subitem;
	AG_Mapview *mv;
	AG_HPane *pane;
	AG_HPaneDiv *div;
	AG_Scrollbar *hbar, *vbar;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, "%s", AGOBJECT(ckt)->name);

	toolbar = Malloc(sizeof(AG_Toolbar), M_OBJECT);
	AG_ToolbarInit(toolbar, AG_TOOLBAR_VERT, 1, 0);
	statbar = Malloc(sizeof(AG_Statusbar), M_OBJECT);
	AG_StatusbarInit(statbar);
	mv = Malloc(sizeof(AG_Mapview), M_OBJECT);
	AG_MapviewInit(mv, ckt->vg->map,
	    AG_MAPVIEW_NO_BMPSCALE|AG_MAPVIEW_NO_NODESEL|AG_MAPVIEW_EDIT,
	    toolbar, statbar);
	
	menu = AG_MenuNew(win);
	pitem = AG_MenuAddItem(menu, _("File"));
	{
		AG_MenuActionKb(pitem, _("Revert"), OBJLOAD_ICON,
		    SDLK_r, KMOD_CTRL, revert_circuit, "%p", ckt);
		AG_MenuActionKb(pitem, _("Save"), OBJSAVE_ICON,
		    SDLK_s, KMOD_CTRL, save_circuit, "%p", ckt);

		subitem = AG_MenuAction(pitem, _("Export circuit"),
		    OBJSAVE_ICON, NULL, NULL);
		AG_MenuAction(subitem, _("Export to SPICE3..."), EDA_SPICE_ICON,
		    export_to_spice, "%p", ckt);
	
		AG_MenuSeparator(pitem);

		AG_MenuActionKb(pitem, _("Close document"), CLOSE_ICON,
		    SDLK_w, KMOD_CTRL, AGWINCLOSE(win));
	}
	pitem = AG_MenuAddItem(menu, _("Edit"));
	{
		/* TODO */
		AG_MenuAction(pitem, _("Undo"), -1, NULL, NULL);
		AG_MenuAction(pitem, _("Redo"), -1, NULL, NULL);

		AG_MenuSeparator(pitem);

		AG_MenuAction(pitem, _("Layout preferences..."), SETTINGS_ICON,
		    layout_settings, "%p,%p", win, ckt);
	}

	pitem = AG_MenuAddItem(menu, _("Drawing"));
	VG_GenericMenu(menu, pitem, ckt->vg, mv);

	pitem = AG_MenuAddItem(menu, _("View"));
	{
		AG_MenuAction(pitem, _("Show interconnects..."), EDA_MESH_ICON,
		    show_interconnects, "%p,%p", win, ckt);

		AG_MenuSeparator(pitem);

		AG_MenuIntFlags(pitem, _("Show origin"), VGORIGIN_ICON,
		    &ckt->vg->flags, VG_VISORIGIN, 0);
		AG_MenuIntFlags(pitem, _("Show grid"), GRID_ICON,
		    &ckt->vg->flags, VG_VISGRID, 0);
		AG_MenuIntFlags(pitem, _("Show extents"), VGBLOCK_ICON,
		    &ckt->vg->flags, VG_VISBBOXES, 0);
		
		AG_MenuSeparator(pitem);
		
		AG_MenuAction(pitem, _("Create view..."), NEW_VIEW_ICON,
		    create_view, "%p,%p,%p", mv, win, ckt);
	}

	pitem = AG_MenuAddItem(menu, _("Simulation"));
	{
		extern const struct sim_ops *sim_ops[];
		const struct sim_ops **ops;

		for (ops = &sim_ops[0]; *ops != NULL; ops++) {
			AG_MenuTool(pitem, toolbar, _((*ops)->name),
			    (*ops)->icon, 0, 0,
			    select_sim, "%p,%p,%p", ckt, *ops, win);
		}
	}
#if 0
	pitem = AG_MenuAddItem(menu, _("Component"));
	component_reg_menu(menu, pitem, ckt, mv);
#endif

	pane = AG_HPaneNew(win, AG_HPANE_WFILL|AG_HPANE_HFILL);
	div = AG_HPaneAddDiv(pane,
	    AG_BOX_VERT, AG_BOX_HFILL,
	    AG_BOX_HORIZ, AG_BOX_WFILL|AG_BOX_HFILL);
	{
		AG_Notebook *nb;
		AG_NotebookTab *ntab;
		AG_Tlist *tl;
		AG_Box *box;

		nb = AG_NotebookNew(div->box1, AG_NOTEBOOK_HFILL|
					       AG_NOTEBOOK_WFILL);
		ntab = AG_NotebookAddTab(nb, _("Models"), AG_BOX_VERT);
		{
			char tname[AG_OBJECT_TYPE_MAX];
			extern const struct eda_type eda_models[];
			const struct eda_type *ty;
			int i;

			tl = AG_TlistNew(ntab, AG_TLIST_POLL|AG_TLIST_TREE);
			AGWIDGET(tl)->flags &= ~(AG_WIDGET_FOCUSABLE);
			AG_SetEvent(tl, "tlist-dblclick", component_insert,
			    "%p,%p,%p", mv, tl, ckt);

			AG_ButtonAct(ntab, _("Insert component"),
			    AG_BUTTON_WFILL, component_insert,
			    "%p,%p,%p", mv, tl, ckt);

			for (ty = &eda_models[0]; ty->name != NULL; ty++) {
				for (i = 0; i < agnTypes; i++) {
					strlcpy(tname, "component.",
					    sizeof(tname));
					strlcat(tname, ty->name, sizeof(tname));
					if (strcmp(agTypes[i].type, tname) == 0)
						break;
				}
				if (i < agnTypes) {
					AG_ObjectType *ctype = &agTypes[i];
					struct component_ops *cops =
					    (struct component_ops *)ctype->ops;

					AG_TlistAddPtr(tl,
					    AGICON(EDA_COMPONENT_ICON),
					    _(cops->name), ctype);
				}
			}
		}

		ntab = AG_NotebookAddTab(nb, _("Objects"), AG_BOX_VERT);
		{
			tl = AG_TlistNew(ntab, AG_TLIST_POLL|AG_TLIST_TREE);
			AG_SetEvent(tl, "tlist-poll", poll_objs, "%p", ckt);
			mv->objs_tl = tl;
			AGWIDGET(tl)->flags &= ~(AG_WIDGET_FOCUSABLE);
		}

		ntab = AG_NotebookAddTab(nb, _("Component"), AG_BOX_VERT);
		{
#if 0
			AG_FSpinbutton *fsb;

			label_static(ntab, _("Temperature:"));

			fsb = AG_FSpinbuttonNew(win, "degC", _("Reference: "));
			AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT,
			    &com->Tnom);
			AG_FSpinbuttonSetMin(fsb, 0.0);
	
			fsb = AG_FSpinbuttonNew(win, "degC", _("Instance: "));
			AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT,
			    &com->Tspec);
			AG_FSpinbuttonSetMin(fsb, 0.0);

			AG_SeparatorNew(ntab, SEPARATOR_HORIZ);
#endif
			AG_LabelNew(ntab, AG_LABEL_STATIC, _("Pinout:"));
			{
				tl = AG_TlistNew(ntab, AG_TLIST_POLL);
				AGWIDGET(tl)->flags &= ~(AG_WIDGET_HFILL);
				AG_SetEvent(tl, "tlist-poll",
				    poll_component_pins, "%p", ckt);
			}
			AG_LabelNew(ntab, AG_LABEL_STATIC, _("Dipoles:"));
			{
				tl = AG_TlistNew(ntab, AG_TLIST_POLL);
				AG_SetEvent(tl, "tlist-poll",
				    poll_component_dipoles, "%p", ckt);
			}
		}

		vbar = AG_ScrollbarNew(div->box2, AG_SCROLLBAR_VERT);
		box = AG_BoxNew(div->box2, AG_BOX_VERT,
		    AG_BOX_WFILL|AG_BOX_HFILL);
		{
			AG_ObjectAttach(box, mv);
			AG_WidgetFocus(mv);
			hbar = AG_ScrollbarNew(box, AG_SCROLLBAR_HORIZ);
		}
		AG_ObjectAttach(div->box2, toolbar);
	}

	{
		extern AG_MaptoolOps component_ins_tool;
		extern AG_MaptoolOps component_sel_tool;
		extern AG_MaptoolOps conductor_tool;
		extern AG_MaptoolOps vgScaleTool;
		extern AG_MaptoolOps vgOriginTool;
		extern AG_MaptoolOps vgLineTool;
		extern AG_MaptoolOps vgTextTool;
		AG_Maptool *seltool;

		AG_MapviewPrescale(mv, 10, 8);
		AG_MapviewRegDrawCb(mv, rasterize_circuit, ckt->vg);
		AG_MapviewUseScrollbars(mv, hbar, vbar);
		
		AG_MapviewRegTool(mv, &component_ins_tool, ckt);
		AG_MapviewRegTool(mv, &conductor_tool, ckt);
		AG_MapviewRegTool(mv, &vgScaleTool, ckt->vg);
		AG_MapviewRegTool(mv, &vgOriginTool, ckt->vg);
		AG_MapviewRegTool(mv, &vgLineTool, ckt->vg);
		AG_MapviewRegTool(mv, &vgTextTool, ckt->vg);
		
		seltool = AG_MapviewRegTool(mv, &component_sel_tool, ckt);
		AG_MapviewSetDefaultTool(mv, seltool);

		AG_SetEvent(mv, "mapview-dblclick", double_click, "%p", ckt);
	}

	AG_ObjectAttach(win, statbar);
	AG_WindowScale(win, -1, -1);
	AG_WindowSetGeometry(win,
	    agView->w/6, agView->h/6,
	    2*agView->w/3, 2*agView->h/3);
	
	return (win);
}
#endif /* EDITION */
