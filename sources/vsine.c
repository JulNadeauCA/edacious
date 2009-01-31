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
 * Independent sinusoidal voltage source.
 */

#include <core/core.h>
#include "sources.h"

const ES_Port esVSinePorts[] = {
	{  0, "" },
	{  1, "v+" },
	{  2, "v-" },
	{ -1 },
};

static __inline__ void
Stamp(ES_VSine *vs, ES_SimDC *dc)
{
	StampVoltageSource(VSOURCE(vs)->v, VSOURCE(vs)->s);
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_VSine *vsine = obj;
	ES_Vsource *vs = VSOURCE(vsine);

	Uint k = PNODE(vsine,1);
	Uint j = PNODE(vsine,2);

	InitStampVoltageSource(k,j, vs->vIdx, vs->s, dc);

	vs->v = 0.0;
	Stamp(vsine,dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_VSine *vs = obj;

	VSOURCE(vs)->v = vs->vPeak*Sin(vs->f * dc->Telapsed);
	
	Stamp(vs,dc);
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_VSine *vs = obj;

	Stamp(vs,dc);
}

static void
Init(void *p)
{
	ES_VSine *vs = p;

	ES_InitPorts(vs, esVSinePorts);

	vs->vPeak = 5.0;
	vs->f = 6.0;
	COMPONENT(vs)->dcSimBegin = DC_SimBegin;
	COMPONENT(vs)->dcStepBegin = DC_StepBegin;
	COMPONENT(vs)->dcStepIter = DC_StepIter;

	M_BindReal(vs, "vPeak", &vs->vPeak);
	M_BindReal(vs, "f", &vs->f);
}

static void *
Edit(void *p)
{
	ES_VSine *vs = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewReal(box, 0, "V", _("Peak voltage: "), &vs->vPeak);
	M_NumericalNewRealPNZ(box, 0, "Hz", _("Frequency: "), &vs->f);
	AG_LabelNewPolledMT(box, 0, &OBJECT(vs)->lock,
	    _("Effective voltage: %[R]"), &VSOURCE(vs)->v);
	return (box);
}

ES_ComponentClass esVSineClass = {
	{
		"Edacious(Circuit:Component:Vsource:VSine)"
		"@sources",
		sizeof(ES_VSine),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		Edit
	},
	N_("Voltage source (sine)"),
	"Vsin",
	"Generic|Sources",
	&esIconVsine,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
