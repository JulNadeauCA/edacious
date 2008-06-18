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
 * LED component model.
 */

#include <eda.h>
#include "led.h"

const ES_Port esLedPorts[] = {
	{ 0, "" },
	{ 1, "A" },
	{ 2, "C" },
	{ -1 },
};

static void
Draw(void *p, VG *vg)
{
	ES_Led *r = p;
	VG_Polygon *vp;

	vp = VG_PolygonNew(vg->root);
	if (r->state) {
		VG_SetColorRGB(vp, 200,0,0);
	} else {
		VG_SetColorRGB(vp, 0,0,0);
	}
	VG_PolygonVertex(vp, VG_PointNew(vp, VGVECTOR(0.156, -0.240)));
	VG_PolygonVertex(vp, VG_PointNew(vp, VGVECTOR(0.156, 0.240)));
	VG_PolygonVertex(vp, VG_PointNew(vp, VGVECTOR(1.09375, 0.240)));
	VG_PolygonVertex(vp, VG_PointNew(vp, VGVECTOR(1.09375, -0.240)));
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Led *led = p;

	led->Vforw = M_ReadReal(buf);
	led->Vrev = M_ReadReal(buf);
	led->I = M_ReadReal(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Led *led = p;

	M_WriteReal(buf, led->Vforw);
	M_WriteReal(buf, led->Vrev);
	M_WriteReal(buf, led->I);
	return (0);
}

void
ES_LedUpdate(void *p)
{
	ES_Led *r = p;
	M_Real v1 = ES_NodeVoltage(COMPONENT(r)->ckt,PNODE(r,1));
	M_Real v2 = ES_NodeVoltage(COMPONENT(r)->ckt,PNODE(r,2));

	r->state = ((v1 - v2) >= r->Vrev);
}

static void
Init(void *p)
{
	ES_Led *r = p;

	ES_InitPorts(r, esLedPorts);
	r->Vforw = 30e-3;
	r->Vrev = 5.0;
	r->I = 2500e-3;
	r->state = 0;
	COMPONENT(r)->intUpdate = ES_LedUpdate;
}

static void *
Edit(void *p)
{
	ES_Led *r = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealR(box, 0, "V", _("Forward voltage: "),
	    &r->Vforw, 1.0, HUGE_VAL);
	M_NumericalNewRealR(box, 0, "V", _("Reverse voltage: "),
	    &r->Vrev, 1.0, HUGE_VAL);
	M_NumericalNewReal(box, 0, "mcd", _("Luminous intensity: "),
	    &r->I);
	return (box);
}

ES_ComponentClass esLedClass = {
	{
		"ES_Component:ES_Led",
		sizeof(ES_Led),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Led"),
	"LED",
	NULL,			/* schem */
	"Generic|Nonlinear|Optical",
	&esIconLED,
	Draw,
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
