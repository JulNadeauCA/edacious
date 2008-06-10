/*
 * Copyright (c) 2006-2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Reference ground component. Whatever node is attached to this component is
 * replaced by node 0.
 */

#include <eda.h>
#include "ground.h"

const ES_Port esGroundPorts[] = {
	{ 0, "", },
	{ 1, "A", },
	{ -1 },
};

static int
Connect(void *p, ES_Port *p1, ES_Port *p2)
{
	ES_Ground *gnd = p;
	ES_Circuit *ckt = COM(gnd)->ckt;
	ES_Branch *br;

	if (p2 != NULL && p2->node > 0) {
		Uint nOld = p2->node;
		ES_Node *node = ckt->nodes[nOld];
rescan:
		NODE_FOREACH_BRANCH(br, node) {
			ES_Port *brPort = br->port;
			
			if (brPort == NULL || brPort->com == NULL ||
			    COMPONENT_IS_FLOATING(brPort->com)) {
				continue;
			}
			ES_CircuitLog(ckt,
			    _("Grounding %s(%s) (previously on n%d)"),
			    OBJECT(brPort->com)->name, brPort->name,
			    brPort->node);
			brPort->node = 0;
			ES_AddBranch(ckt, 0, brPort);
			ES_DelBranch(ckt, nOld, br);
			goto rescan;
		}
		ES_DelNode(ckt, nOld);
	}
	p1->node = 0;
	p1->branch = ES_AddBranch(ckt, 0, p1);
	return (0);
}

static void
Init(void *p)
{
	ES_Ground *gnd = p;

	ES_InitPorts(gnd, esGroundPorts);
}

ES_ComponentClass esGroundClass = {
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
	"Ground.eschem",
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	Connect
};
