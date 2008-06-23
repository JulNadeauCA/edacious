/*
 * Copyright (c) 2004-2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Model for the capacitor.
 */

#include <eda.h>
#include "capacitor.h"

enum {
	PORT_A = 1,
	PORT_B = 2
};
const ES_Port esCapacitorPorts[] = {
	{ 0, "" },
	{ PORT_A, "A" },
	{ PORT_B, "B" },
	{ -1 },
};

/*
 * Returns the voltage across the diode calculated in the last Newton-Raphson
 * iteration.
 */

static M_Real
v(ES_Capacitor *c)
{
	return VPORT(c,PORT_A)-VPORT(c,PORT_B);
}

/* Updates the small- and large-signal models, saving the previous values. */
static void
UpdateModel(ES_Capacitor *c, M_Real v)
{
	c->Ieq=c->C/(1e-9)*v;
	c->g=c->C/(1e-9);
}

static void
UpdateStamp(ES_Capacitor *c, ES_SimDC *dc)
{
	Uint k = PNODE(c,PORT_A);
	Uint l = PNODE(c,PORT_B);

	StampConductance(c->g - c->gPrev,k,l,dc->G);
	StampCurrentSource(c->Ieq - c->IeqPrev,k,l,dc->i);

	c->gPrev = c->g;
	c->IeqPrev = c->Ieq;
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
        ES_Capacitor *c = obj;
	
	UpdateModel(c, c->V0);
	UpdateStamp(c, dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_Capacitor *c = obj;
	
	UpdateModel(c, v(c));
	UpdateStamp(c, dc);

}

static void
Init(void *p)
{
	ES_Capacitor *c = p;

	ES_InitPorts(c, esCapacitorPorts);
	c->C = 1e-8;
	c->V0 = 0;
	
	c->gPrev = 0.0;
	c->IeqPrev = 0.0;

	COMPONENT(c)->dcSimBegin = DC_SimBegin;
	COMPONENT(c)->dcStepBegin = DC_StepBegin;
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Capacitor *c = p;

	c->C = M_ReadReal(buf);
	c->V0 = M_ReadReal(buf);
	
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Capacitor *c = p;

	M_WriteReal(buf, c->C);
	M_WriteReal(buf, c->V0);

	return (0);
}

static void *
Edit(void *p)
{
	ES_Capacitor *c = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);
	AG_Numerical *num;

	M_NumericalNewRealPNZ(box, 0, "uF", _("Capacitance: "), &c->C);
	M_NumericalNewRealPNZ(box, 0, "V", _("Initial voltage: "), &c->V0);

	return (box);
}

ES_ComponentClass esCapacitorClass = {
	{
		"ES_Component:ES_Capacitor",
		sizeof(ES_Capacitor),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,		/* load */
		Save,		/* save */
		Edit
	},
	N_("Capacitor"),
	"C",
	"Capacitor.eschem",
	"Generic|Nonlinear",
	NULL,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
