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

#include <core/core.h>
#include "generic.h"

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
vPrevStep(ES_Inductor *i, int n)
{
	return V_PREV_STEP(i,PORT_A, n)-V_PREV_STEP(i,PORT_B, n);
}
static M_Real
vThisStep(ES_Inductor *i)
{
	return VPORT(i, PORT_A) - VPORT(i, PORT_B);
}


/* Updates the small- and large-signal models, saving the previous values. */
static void
UpdateModel(ES_Inductor *i, ES_SimDC *dc, M_Real v)
{
	switch(dc->method) {
	case BE:
		i->Ieq += v*i->g;
		i->g = dc->deltaT / i->L;
		break;
	case FE:
		i->Ieq += dc->deltaT / i->L * v;
		break;
	case TR:
		i->Ieq += v*i->g + dc->deltaT / (2 * i->L) * v;
		i->g = dc->deltaT / (2 * i->L);
		break;
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}
}

static void
Stamp(ES_Inductor *i, ES_SimDC *dc)
{
	switch(dc->method) {
	case BE:
		StampConductance(i->g, i->s_conductance);
		StampCurrentSource(i->Ieq, i->s_current_source);
		break;
	case FE:
		StampCurrentSource(i->Ieq,i->s_current_source);
		break;
	case TR:
		StampConductance(i->g, i->s_conductance);
		StampCurrentSource(i->Ieq, i->s_current_source);
		break;
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
        ES_Inductor *i = obj;
	Uint k = PNODE(i,PORT_A);
	Uint l = PNODE(i,PORT_B);

		switch(dc->method) {
	case BE:
		InitStampConductance(k, l, i->s_conductance, dc);
		InitStampCurrentSource(l, k, i->s_current_source, dc);
		break;
	case FE:
		InitStampCurrentSource(l, k, i->s_current_source, dc);
		break;
	case TR:
		InitStampConductance(k, l, i->s_conductance, dc);
		InitStampCurrentSource(l, k, i->s_current_source, dc);
		break;
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}
	
	i->g = 0.0;
	i->Ieq = 0.0;
	Stamp(i, dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_Inductor *i = obj;
	
	UpdateModel(i, dc, vPrevStep(i, 1));
	Stamp(i, dc);

}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_Inductor *i = obj;
	
	Stamp(i, dc);
}

/* Returns current flowing through the linearized model at previous step */
static M_Real
InductorBranchCurrent(ES_Inductor *i, ES_SimDC *dc)
{
	switch(dc->method) {
	case BE:
	case TR:
		return i->Ieq + i->g * vPrevStep(i, 1);
	case FE:
		return i->Ieq;
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}

}

/* LTE for capacitor, estimated via divided differences
 * We use the LTE formula relevant to the integration method, and compute
 * it by approximating the derivative with divided differences */
static void
DC_UpdateError(void *obj, ES_SimDC *dc, M_Real *err)
{
	ES_Inductor *i = obj;
	M_Real localErr;
	M_Real I = InductorBranchCurrent(i, dc);
	switch(dc->method) {
	case BE:
	case FE:
		/* Error = dt*dt/2 * i''(e) = dt/2/L * v'(e) */
		localErr = dc->deltaT / 2.0 / i->L * Fabs((vThisStep(i) - vPrevStep(i, 1))
							  / I);
		break;
	case TR:
	{
		/* Error = (dt^3)/12 i'''(e) = (dt^3)/12/L v''(e) */
		M_Real dtn = dc->deltaT;
		M_Real dtnm1 = dc->deltaTPrevSteps[0];
		
		M_Real vnp1 = vThisStep(i);
		M_Real vn = vPrevStep(i, 1);
		M_Real vnm1 = vPrevStep(i, 2);
		
		M_Real secondDerivative = ((vnp1 - vn)/dtn - (vn - vnm1)/dtnm1);
		secondDerivative /= (dtn + dtnm1);
		
		localErr = dtn * dtn * dtn * secondDerivative
			/ (12.0 * i->L * I);

		localErr = Fabs(localErr);
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
Init(void *p)
{
	ES_Inductor *i = p;

	ES_InitPorts(i, esInductorPorts);
	i->L = 1.0;
	
	COMPONENT(i)->dcSimBegin = DC_SimBegin;
	COMPONENT(i)->dcStepBegin = DC_StepBegin;
	COMPONENT(i)->dcStepIter = DC_StepIter;
	COMPONENT(i)->dcUpdateError = DC_UpdateError;
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

	M_NumericalNewRealPNZ(box, 0, "H", _("Inductance: "), &i->L);

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
	&esIconInductor,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
