/*
 * Copyright (c) 2006 Hypertriton, Inc. <http://hypertriton.com/>
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
#include "ground.h"

const ES_Port esGroundPinout[] = {
	{ 0, "",  0.000, 0.000 },
	{ 1, "A", 0.000, 0.000 },
	{ -1 },
};

static int
Connect(void *p, ES_Port *p1, ES_Port *p2)
{
	ES_Ground *gnd = p;
	ES_Circuit *ckt = COM(gnd)->ckt;
	ES_Branch *br;

	if (p2 != NULL && p2->node > 0) {
		u_int n = p2->node;
		ES_Node *node = ckt->nodes[n];
	
		TAILQ_FOREACH(br, &node->branches, branches) {
			if (br->port == NULL || br->port->com == NULL ||
			    br->port->com->flags & COMPONENT_FLOATING) {
				continue;
			}
			ES_CircuitAddBranch(ckt, 0, br->port);
			br->port->node = 0;
		}
		ES_CircuitDelNode(ckt, n);
	}
	p1->node = 0;
	p1->branch = ES_CircuitAddBranch(ckt, 0, p1);
	return (0);
}

static void
Draw(void *p, VG *vg)
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

static void
Init(void *p)
{
	ES_Ground *gnd = p;

	ES_ComponentSetPorts(gnd, esGroundPinout);
}

const ES_ComponentOps esGroundOps = {
	{
		"ES_Component:ES_Ground",
		sizeof(ES_Ground),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		NULL		/* edit */
	},
	N_("Ground"),
	"Gnd",
	Draw,
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	Connect
};
