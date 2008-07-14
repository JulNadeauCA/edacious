/*
 * Copyright (c) 2005-2008 Hypertriton, Inc. <http://hypertriton.com/>
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

static void
UpdateStamp(ES_VSine *vs, ES_SimDC *dc)
{

	StampVoltageSource(VSOURCE(vs)->v, VSOURCE(vs)->s);
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_VSine *vs = obj;
	Uint k = PNODE(vs,1);
	Uint j = PNODE(vs,2);

	InitStampVoltageSource(k,j, VSOURCE(vs)->vIdx, VSOURCE(vs)->s, dc);

	VSOURCE(vs)->v = 0.0;
	UpdateStamp(vs,dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_VSine *vs = obj;

	VSOURCE(vs)->v = vs->vPeak*Sin(vs->f * dc->Telapsed);
	
	UpdateStamp(vs,dc);
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
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_VSine *vs = p;

	vs->vPeak = M_ReadReal(buf);
	vs->f = M_ReadReal(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_VSine *vs = p;

	M_WriteReal(buf, vs->vPeak);
	M_WriteReal(buf, vs->f);
	return (0);
}

static void *
Edit(void *p)
{
	ES_VSine *vs = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewReal(box, 0, "V", _("Peak voltage: "), &vs->vPeak);
	M_NumericalNewRealPNZ(box, 0, "Hz", _("Frequency: "), &vs->f);
	AG_LabelNewPolledMT(box, 0, &OBJECT(vs)->lock,
	    _("Effective voltage: %f"), &VSOURCE(vs)->v);
	return (box);
}

ES_ComponentClass esVSineClass = {
	{
		"ES_Component:ES_Vsource:ES_VSine",
		sizeof(ES_VSine),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Voltage source (sine)"),
	"Vsin",
	"Sources/Vsine.eschem",
	"Generic|Sources",
	&esIconVsine,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
