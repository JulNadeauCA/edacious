/*
 * Copyright (c) 2008-2020 Julien Nadeau Carriere (vedge@csoft.net)
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
 * Independent sweeping voltage source.
 */

#include <core/core.h>
#include "sources.h"

const ES_Port esVSweepPorts[] = {
	{  0, "" },
	{  1, "v+" },
	{  2, "v-" },
	{ -1 },
};

static __inline__ void
Stamp(ES_VSweep *vsw, ES_SimDC *dc)
{
	StampVoltageSource(ESVSOURCE(vsw)->v, ESVSOURCE(vsw)->s);
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_VSweep *vsw = obj;
	ES_Vsource *vs = ESVSOURCE(vsw);
	const Uint k = PNODE(vsw,1);
	const Uint j = PNODE(vsw,2);

	vs->v = vsw->v1;
	InitStampVoltageSource(k,j, vs->vIdx, vs->s, dc);
	Stamp(vsw,dc);
	vsw->vPrev = vs->v;
	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_VSweep *vsw = obj;
	ES_Vsource *vs = ESVSOURCE(vsw);
	Uint curCycle;

	curCycle = dc->Telapsed / vsw->t;
	if (vsw->count != 0 && curCycle >= vsw->count)
		vs->v = 0.0;
	else {
		/* fraction representing the progress in the current cycle */
		const M_Real relProgress = dc->Telapsed / vsw->t - curCycle;

		vs->v = vsw->v1 + (vsw->v2 - vsw->v1) * relProgress;
	}

	if (M_Fabs(vs->v-vsw->vPrev) > 0.5)
		dc->inputStep = 1;
	
	vsw->vPrev = vs->v;

	Stamp(vsw,dc);
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_VSweep *vsw = obj;
	Stamp(vsw,dc);
}

static void
Init(void *p)
{
	ES_VSweep *vsw = p;

	ES_InitPorts(vsw, esVSweepPorts);
	vsw->v1 = 0.0;
	vsw->v2 = 5.0;
	vsw->t = 1.0;
	vsw->count = 1;
	COMPONENT(vsw)->dcSimBegin = DC_SimBegin;
	COMPONENT(vsw)->dcStepBegin = DC_StepBegin;
	COMPONENT(vsw)->dcStepIter = DC_StepIter;

	M_BindReal(vsw, "v1", &vsw->v1);
	M_BindReal(vsw, "v2", &vsw->v2);
	M_BindReal(vsw, "t", &vsw->t);
	AG_BindInt(vsw, "count", &vsw->count);
}

static void *
Edit(void *p)
{
	ES_VSweep *vsw = p;
	AG_Box *box;

	box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);
	M_NumericalNewReal(box, 0, "V", _("Start voltage: "), &vsw->v1);
	M_NumericalNewReal(box, 0, "V", _("End voltage: "), &vsw->v2);
	M_NumericalNewReal(box, 0, "s", _("Duration: "), &vsw->t);
	AG_NumericalNewInt(box, 0, NULL, _("Count (0=loop): "), &vsw->count);
	return (box);
}

ES_ComponentClass esVSweepClass = {
	{
		"Edacious(Circuit:Component:Vsource:VSweep)"
		"@sources",
		sizeof(ES_VSweep),
		{ 0,0 },
		Init,
		NULL,		/* free_dataset */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		Edit
	},
	N_("Voltage source (sweep)"),
	"Vsw",
	"Generic|Sources",
	&esIconComponent,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
