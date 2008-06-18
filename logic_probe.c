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
 * Ideal logic probe.
 */

#include <eda.h>
#include "logic_probe.h"

const ES_Port esLogicProbePorts[] = {
	{ 0, "" },
	{ 1, "A" },
	{ -1 },
};

static void
Draw(void *p, VG *vg)
{
	ES_LogicProbe *r = p;
	VG_Polygon *vp;
	
	vp = VG_PolygonNew(vg->root);
	if (r->state) {
		VG_SetColorRGB(vp, 200,0,0);
	} else {
		VG_SetColorRGB(vp, 0,0,0);
	}
	VG_PolygonVertex(vp, VG_PointNew(vp, VGVECTOR(0.156, -0.125)));
	VG_PolygonVertex(vp, VG_PointNew(vp, VGVECTOR(0.156,  0.125)));
	VG_PolygonVertex(vp, VG_PointNew(vp, VGVECTOR(0.500,  0.125)));
	VG_PolygonVertex(vp, VG_PointNew(vp, VGVECTOR(0.500, -0.125)));
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_LogicProbe *lp = p;

	lp->Vhigh = M_ReadReal(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_LogicProbe *lp = p;

	M_WriteReal(buf, lp->Vhigh);
	return (0);
}

void
ES_LogicProbeUpdate(void *p)
{
	ES_LogicProbe *r = p;
	M_Real v1 = ES_NodeVoltage(COMPONENT(r)->ckt,PNODE(r,1));
	M_Real v2 = ES_NodeVoltage(COMPONENT(r)->ckt,PNODE(r,2));

	r->state = ((v1 - v2) >= r->Vhigh);
}

static void
Init(void *p)
{
	ES_LogicProbe *r = p;

	ES_InitPorts(r, esLogicProbePorts);
	r->Vhigh = 5.0;
	r->state = 0;
	COMPONENT(r)->intUpdate = ES_LogicProbeUpdate;
}

static void *
Edit(void *p)
{
	ES_LogicProbe *r = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealR(box, 0, "V", _("HIGH voltage: "),
	    &r->Vhigh, 1.0, HUGE_VAL);
	return (box);
}

ES_ComponentClass esLogicProbeClass = {
	{
		"ES_Component:ES_LogicProbe",
		sizeof(ES_LogicProbe),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Logic Probe"),
	"LPROBE",
	NULL,			/* schem */
	"Generic|Digital|Probes",
	&esIconLogicProbe,
	Draw,
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
