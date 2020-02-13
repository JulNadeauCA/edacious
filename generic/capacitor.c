/*
 * Copyright (c) 2005-2020 Julien Nadeau Carriere (vedge@csoft.net)
 * Copyright (c) 2008 Antoine Levitt (smeuuh@gmail.com)
 * Copyright (c) 2008 Steven Herbst (herbst@mit.edu)
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
GetVoltage(ES_Capacitor *cap, int n)
{
	return V_PREV_STEP(cap, PORT_A, n) -
	       V_PREV_STEP(cap, PORT_B, n);
}

static M_Real
GetCurrent(ES_Capacitor *cap, int n)
{
	return I_PREV_STEP(cap, cap->vIdx, n);
}

static M_Real
GetDeltaT(ES_SimDC *dc, int n)
{
	if (n == 0) {
		return dc->deltaT;
	} else {
		return dc->deltaTPrevSteps[n - 1];
	}
}

static void
UpdateModel(ES_Capacitor *cap, ES_SimDC *dc)
{
	const M_Real v = (dc->currStep == 0) ? cap->V0 : GetVoltage(cap, 1);

	switch (dc->method) {
	case BE:
		/* Thevenin companion model : better suited
		 * for small timesteps */
		cap->v = v;
		cap->r = dc->deltaT / cap->C;
		break;
	case FE:
		cap->v = v + dc->deltaT / cap->C * GetCurrent(cap,1);
		break;
	case TR:
		cap->v = v + dc->deltaT/(2.0 * cap->C) * GetCurrent(cap,1);
		cap->r = dc->deltaT/(2.0 * cap->C);
		break;
	case G2:
		if (dc->currStep == 0) {
			/* Fall back to BE for first step */
			dc->method = BE;
			UpdateModel(cap, dc);
			dc->method = G2;
			return;
		}
		cap->v = 4.0/3.0*GetVoltage(cap,1) -
		         1.0/3.0*GetVoltage(cap,2);
		cap->r = 2.0/3.0*dc->deltaT / cap->C;
		break;
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}
}

static __inline__ void
Stamp(ES_Capacitor *cap, ES_SimDC *dc)
{
	if (ES_IMPLICIT_METHOD(dc->method)) {
		StampThevenin(cap->v, cap->r, cap->s);
	} else {
		StampVoltageSource(cap->v, cap->s);
	}
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
        ES_Capacitor *cap = obj;
	const Uint k = PNODE(cap,PORT_A);
	const Uint l = PNODE(cap,PORT_B);

	if (ES_IMPLICIT_METHOD(dc->method)) {
		InitStampThevenin(k, l, cap->vIdx, cap->s, dc);
	} else {
		InitStampVoltageSource(k, l, cap->vIdx, cap->s, dc);
	}

	UpdateModel(cap, dc);
	Stamp(cap, dc);
	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_Capacitor *cap = obj;
	
	UpdateModel(cap, dc);
	Stamp(cap, dc);
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_Capacitor *cap = obj;
	
	Stamp(cap, dc);
}

/* Computes the approximation of the derivative of v of order
 * (end - start) by divided difference */
static M_Real
DividedDifference(ES_Capacitor *cap, ES_SimDC *dc, int start, int end)
{
	M_Real num, denom;
	int i;

	if (start == end)
		return GetVoltage(cap, start);

	num = (DividedDifference(cap, dc, start+1, end) -
	       DividedDifference(cap, dc, start, end-1)) / (end - start);

	for (i = start, denom = 0.0; i < end; i++) {
		denom += GetDeltaT(dc, i);
	}
	return (num / denom);
}

static M_Real
Derivative(ES_Capacitor *cap, ES_SimDC *dc, int n)
{
	/*
	 * The minus sign is due to the fact that we stock values
	 * from newest to oldest.
	 */
	return - DividedDifference(cap, dc, 0, n);
}

/* LTE for capacitor, estimated via divided differences */
static void
DC_UpdateError(void *obj, ES_SimDC *dc, M_Real *err)
{
	ES_Capacitor *cap = obj;
	M_Real localErr;
	const M_Real dtn = dc->deltaT;
	const ES_IntegrationMethod *im = &esIntegrationMethods[
	    ((dc->method == G2) && (dc->currStep == 0)) ? BE : dc->method];

	localErr = Fabs(
	    Pow(dtn, im->order+1) * im->errConst *
	    Derivative(cap, dc, im->order+1) /
	    GetVoltage(cap, 0));
	
	if (localErr > *err)
		*err = localErr;
}

static void
Connected(AG_Event *event)
{
	ES_Capacitor *cap = ES_CAPACITOR_SELF();
	ES_Circuit *ckt = ESCIRCUIT( AGOBJECT(cap)->parent );

	AG_OBJECT_ISA(ckt, "ES_Circuit:*");

	cap->vIdx = ES_AddVoltageSource(ckt, cap);
}

static void
Disconnected(AG_Event *event)
{
	ES_Capacitor *cap = ES_CAPACITOR_SELF();
	ES_Circuit *ckt = ESCIRCUIT( AGOBJECT(cap)->parent );
	
	AG_OBJECT_ISA(ckt, "ES_Circuit:*");

	ES_DelVoltageSource(ckt, cap->vIdx);
}

static void
Init(void *p)
{
	ES_Capacitor *cap = p;

	ES_InitPorts(cap, esCapacitorPorts);
	cap->vIdx = -1;

	cap->C = 1.0;
	cap->V0 = 0.0;

	COMPONENT(cap)->dcSimBegin = DC_SimBegin;
	COMPONENT(cap)->dcStepBegin = DC_StepBegin;
	COMPONENT(cap)->dcStepIter = DC_StepIter;
	COMPONENT(cap)->dcUpdateError = DC_UpdateError;
	
	AG_SetEvent(cap, "circuit-connected", Connected, NULL);
	AG_SetEvent(cap, "circuit-disconnected", Disconnected, NULL);

	M_BindReal(cap, "C", &cap->C);
	M_BindReal(cap, "V0", &cap->V0);
}

static void *
Edit(void *p)
{
	ES_Capacitor *cap = p;
	AG_Box *box;

	box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);
	M_NumericalNewRealP(box, 0, "F", _("Capacitance: "), &cap->C);
	M_NumericalNewReal(box, 0, "V", _("Initial voltage: "), &cap->V0);

	if (COMCIRCUIT(cap) != NULL) {
		AG_SeparatorNewHoriz(box);
		AG_LabelNewPolled(box, 0, _("Entry in e: %i"), &cap->vIdx);
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
