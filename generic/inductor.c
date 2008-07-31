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

/* Gets voltage at step current-n */
static M_Real
GetVoltage(ES_Inductor *i, int n)
{
	return V_PREV_STEP(i,PORT_A, n)-V_PREV_STEP(i,PORT_B, n);
}

/* Gets current at step current-n */
static M_Real
GetCurrent(ES_Inductor *i, int n)
{
	return i->I[n];
}

/* Returns current flowing through the linearized model at this step */
static M_Real
InductorBranchCurrent(ES_Inductor *i, ES_SimDC *dc)
{
	if(isImplicit[dc->method])
		return i->Ieq + i->g * GetVoltage(i, 1);
	else
		return i->Ieq;
}

static void
CyclePreviousI(ES_Inductor *i, ES_SimDC *dc)
{
	int index;
	if(dc->stepsToKeep == 1)
	{
		i->I[0] = 0.0;
		return;
	}
	
	for(index = dc->stepsToKeep - 2 ; index >= 0 ; index--)
		i->I[index+1] = i->I[index];
	
	i->I[0] = 0.0;
}

static void
UpdateModel(ES_Inductor *i, ES_SimDC *dc)
{
	switch(dc->method) {
	case BE:
		i->Ieq = GetCurrent(i, 1);
		i->g = dc->deltaT / i->L;
		break;
	case FE:
		i->Ieq = GetCurrent(i, 1) + dc->deltaT / i->L * GetVoltage(i, 1);
		break;
	case TR:
		i->Ieq = GetCurrent(i, 1) + dc->deltaT / (2 * i->L) * GetVoltage(i, 1);
		i->g = dc->deltaT / (2 * i->L);
		break;
	case G2:
		if(dc->currStep == 0)
		{
			/* Fall back to BE for first step */
			dc->method = BE;
			UpdateModel(i, dc);
			dc->method = G2;
			return;
		}
		i->Ieq = 4.0/3.0 * GetCurrent(i, 1) - 1.0/3.0 * GetCurrent(i, 2);
		i->g = 2.0/3.0 * dc->deltaT / i->L;
		break;
	default:
		printf("Method %d not implemented\n", dc->method);
		break;
	}
}

static void
Stamp(ES_Inductor *i, ES_SimDC *dc)
{
	if(isImplicit[dc->method]) {
		StampConductance(i->g, i->s_conductance);
		StampCurrentSource(i->Ieq, i->s_current_source);
	}
	else
		StampCurrentSource(i->Ieq,i->s_current_source);
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
        ES_Inductor *i = obj;
	Uint k = PNODE(i,PORT_A);
	Uint l = PNODE(i,PORT_B);
	int index;
	
	if(isImplicit[dc->method]) {
		InitStampConductance(k, l, i->s_conductance, dc);
		InitStampCurrentSource(l, k, i->s_current_source, dc);
	}
	else
		InitStampCurrentSource(l, k, i->s_current_source, dc);

	i->I = malloc(sizeof(M_Real) * dc->stepsToKeep);
	for(index = 0; index < dc->stepsToKeep; index++)
		i->I[index] = 0.0;
	
	i->g = 0.0;
	i->Ieq = 0.0;
	Stamp(i, dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_Inductor *i = obj;
	
	CyclePreviousI(i, dc);
	UpdateModel(i, dc);
	i->I[0] = InductorBranchCurrent(i, dc);

	Stamp(i, dc);

}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_Inductor *i = obj;
	
	Stamp(i, dc);
}


/* Computes third derivative at current point via divided differences */
static M_Real
ThirdDerivative(ES_Inductor *i, ES_SimDC *dc)
{
	/* Actually compute second derivative of v, and use i = L dv/dt */
	M_Real dtn = dc->deltaT;
	M_Real dtnm1 = dc->deltaTPrevSteps[0];
		
	M_Real vnp1 = GetVoltage(i, 0);
	M_Real vn = GetVoltage(i, 1);
	M_Real vnm1 = GetVoltage(i, 2);
		
	M_Real thirdDerivative = ((vnp1 - vn)/dtn - (vn - vnm1)/dtnm1);
	thirdDerivative /= (dtn + dtnm1);
	thirdDerivative /= i->L;
	return thirdDerivative;
}

static M_Real
Derivative(ES_Inductor *i, ES_SimDC *dc, int n)
{
	if(n == 3)
		return ThirdDerivative(i, dc);
	else if(n == 2)
		return (GetVoltage(i, 0) - GetVoltage(i, 1)) / i->L / dc->deltaT;
	else
		return 0.0; /* Not implemented yet */
}

/* LTE for capacitor, estimated via divided differences
 * We use the LTE formula relevant to the integration method, and compute
 * it by approximating the derivative with divided differences */
static void
DC_UpdateError(void *obj, ES_SimDC *dc, M_Real *err)
{
	ES_Inductor *i = obj;
	M_Real localErr;
	M_Real I = GetCurrent(i, 0);
	M_Real dtn = dc->deltaT;

	localErr= Fabs(
		pow(dtn, methodOrder[dc->method] + 1)
		* methodErrorConstant[dc->method]
		* Derivative(i, dc, methodOrder[dc->method] + 1)
		/ GetCurrent(i, 0));

	if(localErr < *err)
		*err = localErr;
}


static void
Init(void *p)
{
	ES_Inductor *i = p;

	ES_InitPorts(i, esInductorPorts);
	i->L = 1.0;
	i->I = NULL;
	
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

static void
Destroy(void *p)
{
	ES_Inductor *i = p;
	if(i->I)
		free(i->I);
}

ES_ComponentClass esInductorClass = {
	{
		"ES_Component:ES_Inductor",
		sizeof(ES_Inductor),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		Destroy,	/* destroy */
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
