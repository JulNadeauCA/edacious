/*
 * Copyright (c) 2008 Antoine Levitt (smeuuh@gmail.com)
 * Copyright (c) 2008 Steven Herbst (herbst@mit.edu)
 * Copyright (c) 2005-2009 Julien Nadeau (vedge@hypertriton.com)
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

#include <core/core.h>
#include "generic.h"

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

static M_Real
GetVoltage(ES_Capacitor *c, int n)
{
	return V_PREV_STEP(c,PORT_A, n)-V_PREV_STEP(c,PORT_B, n);
}

static M_Real
GetCurrent(ES_Capacitor *c, int n)
{
	return I_PREV_STEP(c, c->vIdx, n);
}

static M_Real
GetDeltaT(ES_SimDC *dc, int n)
{
	if(n == 0) return dc->deltaT;
	else return dc->deltaTPrevSteps[n - 1];
}

static void
UpdateModel(ES_Capacitor *c, ES_SimDC *dc)
{
	M_Real v = (dc->currStep == 0) ? c->V0 : GetVoltage(c, 1);

	switch (dc->method) {
	case BE:
		/* Thevenin companion model : better suited
		 * for small timesteps */
		c->v = v;
		c->r = dc->deltaT/c->C;
		break;
	case FE:
		c->v = v + dc->deltaT/c->C*GetCurrent(c,1);
		break;
	case TR:
		c->v = v + dc->deltaT/(2.0*c->C)*GetCurrent(c,1);
		c->r = dc->deltaT/(2.0*c->C);
		break;
	case G2:
		if (dc->currStep == 0) {
			/* Fall back to BE for first step */
			dc->method = BE;
			UpdateModel(c, dc);
			dc->method = G2;
			return;
		}
		c->v = 4.0/3.0*GetVoltage(c,1) - 1.0/3.0*GetVoltage(c,2);
		c->r = 2.0/3.0*dc->deltaT/c->C;
		break;
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}
}

static __inline__ void
Stamp(ES_Capacitor *c, ES_SimDC *dc)
{
	if (ES_IMPLICIT_METHOD(dc->method)) {
		StampThevenin(c->v, c->r, c->s);
	} else {
		StampVoltageSource(c->v, c->s);
	}
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
        ES_Capacitor *c = obj;
	Uint k = PNODE(c,PORT_A);
	Uint l = PNODE(c,PORT_B);

	if (ES_IMPLICIT_METHOD(dc->method)) {
		InitStampThevenin(k, l, c->vIdx, c->s, dc);
	} else {
		InitStampVoltageSource(k, l, c->vIdx, c->s, dc);
	}

	UpdateModel(c, dc);
	Stamp(c, dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_Capacitor *c = obj;
	
	UpdateModel(c, dc);
	Stamp(c, dc);
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_Capacitor *c = obj;
	
	Stamp(c, dc);
}

/* Computes the approximation of the derivative of v of order
 * (end - start) by divided difference */
static M_Real
DividedDifference(ES_Capacitor *c, ES_SimDC *dc, int start, int end)
{
	M_Real num, denom;
	int i;

	if (start == end)
		return GetVoltage(c, start);

	num = (DividedDifference(c, dc, start+1, end) -
	       DividedDifference(c, dc, start, end-1)) /
	       (end-start);
	for (i = start, denom = 0.0; i < end; i++) {
		denom += GetDeltaT(dc, i);
	}
	return (num / denom);
}

static M_Real
Derivative(ES_Capacitor *c, ES_SimDC *dc, int n)
{
	/* the minus sign is due to the fact that we stock values from newest to oldest */
	return - DividedDifference(c, dc, 0, n);
}

/* LTE for capacitor, estimated via divided differences */
static void
DC_UpdateError(void *obj, ES_SimDC *dc, M_Real *err)
{
	ES_Capacitor *c = obj;
	M_Real localErr;
	M_Real dtn = dc->deltaT;
	const ES_IntegrationMethod *im;

	im = &esIntegrationMethods[((dc->method == G2) && (dc->currStep == 0)) ?
	                           BE : dc->method];

	localErr = Fabs(
	    Pow(dtn, im->order+1) * im->errConst *
	    Derivative(c,dc,im->order+1) /
	    GetVoltage(c,0));
	
	if (localErr > *err)
		*err = localErr;
}

static void
Connected(AG_Event *event)
{
	ES_Circuit *ckt = AG_SENDER();
	ES_Capacitor *c = AG_SELF();
	
	c->vIdx = ES_AddVoltageSource(ckt, c);
}

static void
Disconnected(AG_Event *event)
{
	ES_Circuit *ckt = AG_SENDER();
	ES_Capacitor *c = AG_SELF();

	ES_DelVoltageSource(ckt, c->vIdx);
}

static void
Init(void *p)
{
	ES_Capacitor *c = p;

	ES_InitPorts(c, esCapacitorPorts);
	c->vIdx = -1;

	c->C = 1.0;
	c->V0 = 0.0;

	COMPONENT(c)->dcSimBegin = DC_SimBegin;
	COMPONENT(c)->dcStepBegin = DC_StepBegin;
	COMPONENT(c)->dcStepIter = DC_StepIter;
	COMPONENT(c)->dcUpdateError = DC_UpdateError;
	
	AG_SetEvent(c, "circuit-connected", Connected, NULL);
	AG_SetEvent(c, "circuit-disconnected", Disconnected, NULL);

	M_BindReal(c, "C", &c->C);
	M_BindReal(c, "V0", &c->V0);
}

static void *
Edit(void *p)
{
	ES_Capacitor *c = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealP(box, 0, "F", _("Capacitance: "), &c->C);
	M_NumericalNewReal(box, 0, "V", _("Initial voltage: "), &c->V0);

	if (COMCIRCUIT(c) != NULL) {
		AG_SeparatorNewHoriz(box);
		AG_LabelNewPolled(box, 0, _("Entry in e: %i"), &c->vIdx);
	}

	return (box);
}

ES_ComponentClass esCapacitorClass = {
	{
		"Edacious(Circuit:Component:Capacitor)"
		"@generic",
		sizeof(ES_Capacitor),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		Edit
	},
	N_("Capacitor"),
	"C",
	"Generic|Nonlinear",
	&esIconCapacitor,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
