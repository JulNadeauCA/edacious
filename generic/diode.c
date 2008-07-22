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
 * Model for the diode.
 */

#include <core/core.h>
#include "generic.h"

enum {
	PORT_P = 1,
	PORT_N = 2
};
const ES_Port esDiodePorts[] = {
	{ 0, "" },
	{ PORT_P, "P" },	/* P-side (+) */
	{ PORT_N, "N" },	/* N-side (-) */
	{ -1 },
};

/*
 * Returns the voltage across the diode calculated in the last Newton-Raphson
 * iteration.
 */

static M_Real
v(ES_Diode *d)
{
	return VPORT(d,PORT_P)-VPORT(d,PORT_N);
}

static M_Real
vPrevStep(ES_Diode *d)
{
	return V_PREV_STEP(d,PORT_P)-V_PREV_STEP(d,PORT_N);
}

static void
ResetModel(ES_Diode *d)
{
	d->g=1.0;

	d->Ieq=0.0;

	d->vPrevIter = 0.7;

}

/* Updates the small- and large-signal models, saving the previous values. */
static void
UpdateModel(ES_Diode *d, ES_SimDC *dc, M_Real v)
{
	M_Real vDiff = v - d->vPrevIter;
	M_Real I;

	if (Fabs(vDiff) > d->Vt) {
		v = d->vPrevIter + vDiff/Fabs(vDiff)*d->Vt;
		dc->isDamped = 1;
	}

	d->vPrevIter = v;

	I = d->Is*(Exp(v/(d->Vt)) - 1);
	d->g = I/(d->Vt);
	d->Ieq = I-(d->g)*v;
}

static void
Stamp(ES_Diode *d, ES_SimDC *dc)
{
	StampConductance(d->g, d->s_conductance);
	StampCurrentSource(d->Ieq, d->s_current_source);
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
        ES_Diode *d = obj;

	Uint k = PNODE(d,PORT_P);
	Uint l = PNODE(d,PORT_N);

	InitStampConductance(k, l, d->s_conductance, dc);
	InitStampCurrentSource(l, k, d->s_current_source, dc);

	ResetModel(d);
	Stamp(d, dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_Diode *d = obj;

	if (dc->inputStep)
		ResetModel(d);
	else
	{	
		d->vPrevIter = vPrevStep(d);
		UpdateModel(d, dc, vPrevStep(d));
	}

	Stamp(d, dc);
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_Diode *d = obj;

	UpdateModel(d, dc, v(d));
	Stamp(d, dc);
}

static void
Init(void *p)
{
	ES_Diode *d = p;

	ES_InitPorts(d, esDiodePorts);
	d->Is = 1e-14;
	d->Vt = 0.025;
	
	COMPONENT(d)->dcSimBegin = DC_SimBegin;
	COMPONENT(d)->dcStepBegin = DC_StepBegin;
	COMPONENT(d)->dcStepIter = DC_StepIter;
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Diode *d = p;

	d->Is = M_ReadReal(buf);
	d->Vt = M_ReadReal(buf);
	
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Diode *d = p;

	M_WriteReal(buf, d->Is);
	M_WriteReal(buf, d->Vt);

	return (0);
}

static void *
Edit(void *p)
{
	ES_Diode *d = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealR(box, 0, "pA",
	    _("Reverse saturation current: "), &d->Is, M_TINYVAL, HUGE_VAL);
	M_NumericalNewRealPNZ(box, 0, "mV",
	    _("Thermal voltage: "), &d->Vt);

	return (box);
}

ES_ComponentClass esDiodeClass = {
	{
		"ES_Component:ES_Diode",
		sizeof(ES_Diode),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,		/* load */
		Save,		/* save */
		Edit
	},
	N_("Diode"),
	"D",
	"Diode.eschem",
	"Generic|Nonlinear",
	&esIconDiode,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
