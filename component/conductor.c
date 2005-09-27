/*	$Csoft: conductor.c,v 1.5 2005/09/15 02:04:49 vedge Exp $	*/

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

#include <engine/vg/vg.h>

#ifdef EDITION
#include <engine/widget/window.h>
#include <engine/widget/fspinbutton.h>

#include <engine/map/mapview.h>
#include <engine/map/tool.h>
#endif

#include "conductor.h"

const AG_Version conductor_ver = {
	"agar-eda conductor",
	0, 0
};

const struct component_ops conductor_ops = {
	{
		conductor_init,
		NULL,			/* reinit */
		component_destroy,
		NULL,			/* load */
		NULL,			/* save */
		component_edit
	},
	N_("Conductor"),
	"Cd",
	conductor_draw,
	NULL,			/* edit */
	NULL,			/* connect */
	conductor_export,
	NULL,			/* tick */
	conductor_resistance,
	NULL,			/* capacitance */
	NULL,			/* inductance */
	NULL			/* isource */
};

const struct pin conductor_pinout[] = {
	{ 0, "",  0, 0 },
	{ 1, "A", 0, 0 },
	{ 2, "B", 0, 0 },
	{ -1 },
};

void
conductor_draw(void *p, VG *vg)
{
	struct conductor *cd = p;

	VG_Begin(vg, VG_LINE_STRIP);
	VG_Vertex2(vg, COM(cd)->pin[1].x, COM(cd)->pin[1].y);
	VG_Vertex2(vg, COM(cd)->pin[2].x, COM(cd)->pin[2].y);
	VG_End(vg);
}

void
conductor_init(void *p, const char *name)
{
	struct conductor *cd = p;

	component_init(cd, "conductor", name, &conductor_ops, conductor_pinout);
}

double
conductor_resistance(void *p, struct pin *p1, struct pin *p2)
{
	return (0);
}

int
conductor_export(void *p, enum circuit_format fmt, FILE *f)
{
	struct conductor *c = p;
	
	if (PNODE(c,1) == -1 ||
	    PNODE(c,2) == -1)
		return (0);

	switch (fmt) {
	case CIRCUIT_SPICE3:
		fprintf(f, "V%s %d %d 0\n", AGOBJECT(c)->name,
		    PNODE(c,1), PNODE(c,2));
		break;
	}
	return (0);
}

#ifdef EDITION
static struct conductor *sel_cd;

static void
conductor_tool_init(void *p)
{
	/* XXX */
	sel_cd = NULL;
}

void
conductor_tool_reinit(void)
{
	if (sel_cd != NULL) {
		dprintf("non-null conductor\n");
	}
	sel_cd = NULL;
}

static int
conductor_tool_mousebuttondown(void *p, int xmap, int ymap, int b)
{
	AG_Maptool *t = p;
	char name[AG_OBJECT_NAME_MAX];
	struct circuit *ckt = t->p;
	VG *vg = ckt->vg;
	struct component *com;
	double x, y;
	int n = 0;

	switch (b) {
	case SDL_BUTTON_LEFT:
		VG_Map2Vec(vg, xmap, ymap, &x, &y);
		if (sel_cd == NULL) {
tryname:
			snprintf(name, sizeof(name), "Cd%d", n++);
			AGOBJECT_FOREACH_CHILD(com, ckt, component) {
				if (strcmp(AGOBJECT(com)->name, name) == 0)
					break;
			}
			if (com != NULL) {
				goto tryname;
			}
			com = Malloc(sizeof(struct conductor), M_OBJECT);
			conductor_init(com, name);
			AG_ObjectAttach(ckt, com);
			AG_ObjectPageIn(com, AG_OBJECT_DATA);
			AG_PostEvent(ckt, com, "circuit-shown", NULL);

			VG_MoveBlock(vg, com->block, x, y, -1);
			com->pin[1].x = 0;
			com->pin[1].y = 0;
			sel_cd = (struct conductor *)com;
		} else {
			COM(sel_cd)->pin[1].selected = 0;
			COM(sel_cd)->pin[2].selected = 0;
			component_connect(ckt, COM(sel_cd),
			    &COM(sel_cd)->block->pos);
			sel_cd = NULL;
		}
		break;
	case SDL_BUTTON_MIDDLE:
		if (sel_cd != NULL) {
			AG_ObjectDetach(sel_cd);
			AG_ObjectDestroy(sel_cd);
			Free(sel_cd, M_OBJECT);
			sel_cd = NULL;
		}
		break;
	default:
		return (0);
	}
	return (1);
}

static int
conductor_tool_mousemotion(void *p, int xmap, int ymap, int xrel, int yrel,
    int b)
{
	AG_Maptool *t = p;
	struct circuit *ckt = t->p;
	VG *vg = ckt->vg;
	VG_Vtx vtx;

	if (VG_Map2Vec(vg, xmap, ymap, &vtx.x, &vtx.y) == -1) {
		return (0);
	}
	vg->origin[1].x = vtx.x;
	vg->origin[1].y = vtx.y;

	if (sel_cd != NULL) {
		COM(sel_cd)->pin[2].x = vtx.x - COM(sel_cd)->block->pos.x;
		COM(sel_cd)->pin[2].y = vtx.y - COM(sel_cd)->block->pos.y;
		component_highlight_pins(ckt, COM(sel_cd));
	} else {
		if (pin_overlap(ckt, NULL, vtx.x, vtx.y) != NULL) {
			vg->origin[2].x = vtx.x;
			vg->origin[2].y = vtx.y;
		}
	}
	vg->redraw++;
	return (0);
}

AG_MaptoolOps conductor_tool = {
	N_("Conductor"),
	N_("Insert an ideal conductor between two nodes."),
	EDA_BRANCH_TO_NODE_ICON,
	sizeof(AG_Maptool),
	0,
	conductor_tool_init,
	NULL,				/* destroy */
	NULL,				/* load */
	NULL,				/* save */
	NULL,				/* cursor */
	NULL,				/* effect */
	conductor_tool_mousemotion,
	conductor_tool_mousebuttondown,
	NULL,				/* mousebuttonup */
	NULL,				/* keydown */
	NULL				/* keyup */
};
#endif /* EDITION */
