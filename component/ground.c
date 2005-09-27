/*	$Csoft: ground.c,v 1.2 2005/09/15 02:04:49 vedge Exp $	*/

/*
 * Copyright (c) 2005 CubeSoft Communications, Inc.
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
#include <engine/widget/spinbutton.h>
#include <engine/widget/fspinbutton.h>
#endif

#include "ground.h"

const AG_Version ground_ver = {
	"agar-eda ground",
	0, 0
};

const struct component_ops ground_ops = {
	{
		ground_init,
		NULL,			/* reinit */
		component_destroy,
		ground_load,
		ground_save,
		component_edit
	},
	N_("Ground"),
	"Gnd",
	ground_draw,
	NULL,			/* edit */
	ground_connect,
	NULL,			/* export */
	NULL,			/* tick */
	NULL,			/* resistance */
	NULL,			/* capacitance */
	NULL,			/* inductance */
	NULL			/* isource */
};

const struct pin ground_pinout[] = {
	{ 0, "",  0.000, 0.000 },
	{ 1, "A", 0.000, 0.000 },
	{ -1 },
};

int
ground_connect(void *p, struct pin *p1, struct pin *p2)
{
	struct ground *gnd = p;
	struct circuit *ckt = COM(gnd)->ckt;
	struct cktbranch *br;

	if (p2 != NULL && p2->node > 0) {
		u_int n = p2->node;
		struct cktnode *node = ckt->nodes[n];
	
		TAILQ_FOREACH(br, &node->branches, branches) {
			if (br->pin == NULL || br->pin->com == NULL ||
			    br->pin->com->flags & COMPONENT_FLOATING) {
				continue;
			}
			circuit_add_branch(ckt, 0, br->pin);
			br->pin->node = 0;
		}
		circuit_del_node(ckt, n);
	}
	p1->node = 0;
	p1->branch = circuit_add_branch(ckt, 0, p1);
	return (0);
}

void
ground_draw(void *p, VG *vg)
{
	VG_Begin(vg, VG_LINES);
	VG_Vertex2(vg, 0.000, 0.000);
	VG_Vertex2(vg, 0.000, 0.250);
	VG_Vertex2(vg, -0.250, 0.250);
	VG_Vertex2(vg, +0.250, 0.250);
	VG_Vertex2(vg, -0.200, 0.375);
	VG_Vertex2(vg, +0.200, 0.375);
	VG_Vertex2(vg, -0.150, 0.500);
	VG_Vertex2(vg, +0.150, 0.500);
	VG_End(vg);
}

void
ground_init(void *p, const char *name)
{
	struct ground *gnd = p;

	component_init(gnd, "ground", name, &ground_ops, ground_pinout);
}

int
ground_load(void *p, AG_Netbuf *buf)
{
	struct ground *gnd = p;

	if (AG_ReadVersion(buf, &ground_ver, NULL) == -1 ||
	    component_load(gnd, buf) == -1) {
		return (-1);
	}
	return (0);
}

int
ground_save(void *p, AG_Netbuf *buf)
{
	struct ground *gnd = p;

	AG_WriteVersion(buf, &ground_ver);
	if (component_save(gnd, buf) == -1) {
		return (-1);
	}
	return (0);
}

