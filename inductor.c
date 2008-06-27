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
 * Model for the inductor.
 */

#include <eda.h>
#include "inductor.h"

enum {
	PORT_A = 1,
	PORT_B = 2
};
const ES_Port esInductorPorts[] = {
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
v(ES_Inductor *i)
{
	return VPORT(i,PORT_A)-VPORT(i,PORT_B);
}

/* Updates the small- and large-signal models, saving the previous values. */
static void
UpdateModel(ES_Inductor *i, ES_SimDC *dc, M_Real v)
{
	switch(dc->method) {
	case BE:
		i->Ieq = i->IeqPrev + v*i->gPrev;
		i->g = dc->deltaT/i->L;
		break;
	case FE:
		break;
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}
}

static void
UpdateStamp(ES_Inductor *i, ES_SimDC *dc)
{
	Uint k = PNODE(i,PORT_A);
	Uint l = PNODE(i,PORT_B);
	
	switch(dc->method) {
	case BE:
		StampConductance(i->g-i->gPrev,k,l,dc->G);
		StampCurrentSource(i->Ieq-i->IeqPrev,l,k,dc->i);
		break;
	case FE:
		break;
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}

	i->gPrev = i->g;
	i->IeqPrev = i->Ieq;
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
        ES_Inductor *i = obj;

	i->g = 0.01;
	i->Ieq = 0.0;
	UpdateStamp(i, dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_Inductor *i = obj;
	
	UpdateModel(i, dc, v(i));
	UpdateStamp(i, dc);

}

static void
Init(void *p)
{
	ES_Inductor *i = p;

	ES_InitPorts(i, esInductorPorts);
	i->L = 1.0;
	
	i->gPrev = 0.0;
	i->IeqPrev = 0.0;

	COMPONENT(i)->dcSimBegin = DC_SimBegin;
	COMPONENT(i)->dcStepBegin = DC_StepBegin;
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Inductor *i = p;

	i->L = M_ReadReal(buf);
	
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Inductor *i = p;

	M_WriteReal(buf, i->L);

	return (0);
}

static void *
Edit(void *p)
{
	ES_Inductor *i = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);
	AG_Numerical *num;

	M_NumericalNewRealPNZ(box, 0, "uH", _("Inductance: "), &i->L);

	return (box);
}

ES_ComponentClass esInductorClass = {
	{
		"ES_Component:ES_Inductor",
		sizeof(ES_Inductor),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,		/* load */
		Save,		/* save */
		Edit
	},
	N_("Inductor"),
	"L",
	"Inductor.eschem",
	"Generic|Nonlinear",
	NULL,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
