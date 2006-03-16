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

#include <agar/core.h>
#include <agar/vg.h>
#include <agar/gui.h>

#include "eda.h"
#include "conductor.h"

const AG_Version esConductorVer = {
	"agar-eda conductor",
	0, 0
};

const ES_ComponentOps esConductorOps = {
	{
		ES_ConductorInit,
		NULL,			/* reinit */
		ES_ComponentDestroy,
		NULL,			/* load */
		NULL,			/* save */
		ES_ComponentEdit
	},
	N_("Conductor"),
	"Cd",
	ES_ConductorDraw,
	NULL,			/* edit */
	NULL,			/* menu */
	NULL,			/* connect */
	NULL,			/* disconnect */
	ES_ConductorExport
};

const ES_Port esConductorPinout[] = {
	{ 0, "",  0, 0 },
	{ 1, "A", 0, 0 },
	{ 2, "B", 0, 0 },
	{ -1 },
};

void
ES_ConductorDraw(void *p, VG *vg)
{
	ES_Conductor *cd = p;

	VG_Begin(vg, VG_LINE_STRIP);
	VG_Vertex2(vg, COM(cd)->ports[1].x, COM(cd)->ports[1].y);
	VG_Vertex2(vg, COM(cd)->ports[2].x, COM(cd)->ports[2].y);
	VG_End(vg);
}

static int
ES_ConductorLoadDC_G(void *p, SC_Matrix *G)
{
	ES_Conductor *cd = p;
	ES_Node *n;
	u_int k = PNODE(cd,1);
	u_int j = PNODE(cd,2);

	if (k == -1 || j == -1 || (k == 0 && j == 0)) {
		AG_SetError("bad conductor");
		return (-1);
	}
	if (k != 0) { G->mat[k][k] += 0.999; }
	if (j != 0) { G->mat[j][j] += 0.999; }
	if (k != 0 && j != 0) {
		G->mat[k][j] -= 0.999;
		G->mat[j][k] -= 0.999;
	}
	return (0);
}

void
ES_ConductorInit(void *p, const char *name)
{
	ES_Conductor *cd = p;

	ES_ComponentInit(cd, "conductor", name, &esConductorOps,
	    esConductorPinout);
	COM(cd)->loadDC_G = ES_ConductorLoadDC_G;
}

int
ES_ConductorExport(void *p, enum circuit_format fmt, FILE *f)
{
	ES_Conductor *c = p;
	
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
static ES_Conductor *esCurCd;

static void
ES_ConductorToolInit(void *p)
{
	/* XXX */
	esCurCd = NULL;
}

void
ES_ConductorToolReinit(void)
{
	if (esCurCd != NULL) {
		printf("non-null conductor\n");
	}
	esCurCd = NULL;
}

static int
ES_ConductorConnect(ES_Circuit *ckt, ES_Conductor *cd, float x1, float y1,
    float x2, float y2)
{
	ES_Port *port1, *port2;
	int N1, N2, N3;
	int i;

	ES_LockCircuit(ckt);

	port1 = ES_ComponentPortOverlap(ckt, COM(cd), x1, y1);
	port2 = ES_ComponentPortOverlap(ckt, COM(cd), x2, y2);
	if (port1 != NULL && port2 != NULL &&
	    port1->node == port2->node) {
		AG_SetError(_("Redundant connection"));
		goto fail;
	}
	N1 = (port1 != NULL) ? port1->node : ES_CircuitAddNode(ckt, 0);
	N2 = (port2 != NULL) ? port2->node : ES_CircuitAddNode(ckt, 0);
	COM(cd)->ports[1].node = N1;
	COM(cd)->ports[2].node = N2;

	if ((N3 = ES_CircuitMergeNodes(ckt, N1, N2)) == -1) {
		goto fail;
	}
	port1->node = N3;
	port2->node = N3;

	ES_CircuitModified(ckt);
	ES_UnlockCircuit(ckt);
	return (0);
fail:
	ES_UnlockCircuit(ckt);
	return (-1);
}

static int
ES_ConductorToolMousebuttondown(void *p, float x, float y, int b)
{
	VG_Tool *t = p;
	char name[AG_OBJECT_NAME_MAX];
	ES_Circuit *ckt = t->p;
	VG *vg = ckt->vg;
	ES_Component *com;
	int n = 0;

	switch (b) {
	case SDL_BUTTON_LEFT:
		if (esCurCd == NULL) {
tryname:
			snprintf(name, sizeof(name), "Cd%d", n++);
			AGOBJECT_FOREACH_CHILD(com, ckt, es_component) {
				if (strcmp(AGOBJECT(com)->name, name) == 0)
					break;
			}
			if (com != NULL) {
				goto tryname;
			}
			com = Malloc(sizeof(ES_Conductor), M_OBJECT);
			ES_ConductorInit(com, name);
			AG_ObjectAttach(ckt, com);
			AG_ObjectPageIn(com, AG_OBJECT_DATA);
			AG_PostEvent(ckt, com, "circuit-shown", NULL);

			VG_MoveBlock(vg, com->block, x, y, -1);
			com->ports[1].x = 0;
			com->ports[1].y = 0;
			esCurCd = (ES_Conductor *)com;
		} else {
			com = (ES_Component *)esCurCd;
			com->ports[1].selected = 0;
			com->ports[2].selected = 0;
			if (ES_ConductorConnect(ckt, esCurCd,
			    com->block->pos.x + com->ports[1].x,
			    com->block->pos.y + com->ports[1].y,
			    com->block->pos.x + com->ports[2].x,
			    com->block->pos.y + com->ports[2].y) == -1) {
				AG_TextMsg(AG_MSG_ERROR, "%s", AG_GetError());
			} else {
				esCurCd = NULL;
			}
		}
		break;
	default:
		if (esCurCd != NULL) {
			AG_ObjectDetach(esCurCd);
			AG_ObjectDestroy(esCurCd);
			Free(esCurCd, M_OBJECT);
			esCurCd = NULL;
		}
		break;
	}
	return (1);
}

static int
ES_ConductorToolMousemotion(void *p, float x, float y, float xrel, float yrel,
    int b)
{
	VG_Tool *t = p;
	ES_Circuit *ckt = t->p;
	VG *vg = ckt->vg;

	vg->origin[1].x = x;
	vg->origin[1].y = y;

	if (esCurCd != NULL) {
		COM(esCurCd)->ports[2].x = x - COM(esCurCd)->block->pos.x;
		COM(esCurCd)->ports[2].y = y - COM(esCurCd)->block->pos.y;
		ES_ComponentHighlightPorts(ckt, COM(esCurCd));
	} else {
		if (ES_ComponentPortOverlap(ckt, NULL, x, y) != NULL) {
			vg->origin[2].x = x;
			vg->origin[2].y = y;
		}
	}
	return (0);
}

VG_ToolOps esConductorTool = {
	N_("Conductor"),
	N_("Insert an ideal conductor between two nodes."),
	EDA_BRANCH_TO_NODE_ICON,
	sizeof(VG_Tool),
	0,
	ES_ConductorToolInit,
	NULL,				/* destroy */
	NULL,				/* edit */
	ES_ConductorToolMousemotion,
	ES_ConductorToolMousebuttondown,
	NULL,				/* mousebuttonup */
	NULL,				/* keydown */
	NULL				/* keyup */
};
#endif /* EDITION */
