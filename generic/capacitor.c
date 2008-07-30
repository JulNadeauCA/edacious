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

/*
 * Returns the voltage across the diode calculated in the last Newton-Raphson
 * iteration.
 */
static M_Real
vPrevStep(ES_Capacitor *c, int n)
{
	return V_PREV_STEP(c,PORT_A, n)-V_PREV_STEP(c,PORT_B, n);
}

static M_Real
vThisStep(ES_Capacitor *c)
{
	return VPORT(c,PORT_A)-VPORT(c,PORT_B);
}

static M_Real
iPrevStep(ES_Capacitor *c, int n)
{
	return I_PREV_STEP(c, c->vIdx, n);
}

static M_Real
iThisStep(ES_Capacitor *c)
{
	return IBRANCH(c, c->vIdx);
}


/* Updates the small- and large-signal models, saving the previous values. */
static void
UpdateModel(ES_Capacitor *c, M_Real v, ES_SimDC *dc)
{
	switch(dc->method) {
	case BE:
		/* Thevenin companion model : better suited
		 * for small timesteps */
		c->v = v;
		c->r = dc->deltaT / c->C;
		break;
	case FE:
		c->v += dc->deltaT / c->C * iPrevStep(c, 1);
		break;
	case TR:
		c->v = v + dc->deltaT / (2 * c->C) * iPrevStep(c, 1);
		c->r = dc->deltaT / (2 * c->C);
		break;
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}
}

static void
Stamp(ES_Capacitor *c, ES_SimDC *dc)
{
	switch(dc->method) {
	case BE:
		StampThevenin(c->v, c->r, c->s);
		break;
	case FE:
		StampVoltageSource(c->v, c->s);
		break;
	case TR:
		StampThevenin(c->v, c->r, c->s);
		break;
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
        ES_Capacitor *c = obj;
	Uint k = PNODE(c,PORT_A);
	Uint l = PNODE(c,PORT_B);
	
	switch(dc->method) {
	case BE:
		InitStampThevenin(k, l, c->vIdx, c->s, dc);
		break;
	case FE:
		InitStampVoltageSource(k, l, c->vIdx, c->s, dc);
		break;
	case TR:
		InitStampThevenin(k, l, c->vIdx, c->s, dc);
		break;
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}
	c->v = c->V0;
	UpdateModel(c, c->V0, dc);
	Stamp(c, dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_Capacitor *c = obj;
	
	UpdateModel(c, vPrevStep(c, 1), dc);
	Stamp(c, dc);
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_Capacitor *c = obj;
	
	Stamp(c, dc);
}

/* LTE for capacitor, estimated via divided differences
 * We use the LTE formula relevant to the integration method, and compute
 * it by approximating the derivative with divided differences */
static void
DC_UpdateError(void *obj, ES_SimDC *dc, M_Real *err)
{
	ES_Capacitor *c = obj;
	M_Real localErr;
	switch(dc->method) {
	case BE:
	case FE:
		/* Error = dt*dt/2 * v''(e) = dt/2/C * i'(e) */
		localErr = dc->deltaT / 2.0 / c->C * Fabs((iThisStep(c) - iPrevStep(c, 1))
							    / vPrevStep(c, 1));
		break;
	case TR:
	{
		/* Error = (dt^3)/12 v'''(e) */
		M_Real dtn = dc->deltaT;
		M_Real dtnm1 = dc->deltaTPrevSteps[0];
		M_Real dtnm2 = dc->deltaTPrevSteps[1];
		
		M_Real vnp1 = vThisStep(c);
		M_Real vn = vPrevStep(c, 1);
		M_Real vnm1 = vPrevStep(c, 2);
		M_Real vnm2 = vPrevStep(c, 3);
		
		M_Real term1 = (vnp1 - vn)/dtn - (vn - vnm1)/dtnm1;
		M_Real term2 = (vn - vnm1)/dtnm1 - (vnm1 - vnm2)/dtnm2;

		M_Real thirdDerivative = term1 / (dtn + dtnm1) - term2/ (dtnm1 + dtnm2);
		thirdDerivative /= (dtn + dtnm1 + dtnm2);

		localErr = dtn * dtn * dtn * Fabs(thirdDerivative) / 12.0 / vPrevStep(c, 1);

		break;
	}
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}

	if(localErr < *err)
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

	M_NumericalNewRealPNZ(box, 0, "F", _("Capacitance: "), &c->C);
	M_NumericalNewReal(box, 0, "V", _("Initial voltage: "), &c->V0);

	AG_SeparatorNewHoriz(box);
	AG_LabelNewPolled(box, 0, _("Entry in e: %i"), &c->vIdx);

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
	&esIconCapacitor,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
