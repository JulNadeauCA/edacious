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
 * Independent sweeping voltage source.
 */

#include <eda.h>
#include "vsweep.h"

const ES_Port esVSweepPorts[] = {
	{  0, "" },
	{  1, "v+" },
	{  2, "v-" },
	{ -1 },
};

static void
UpdateStamp(ES_VSweep *vsw, ES_SimDC *dc)
{
	Uint k = PNODE(vsw,1);
	Uint j = PNODE(vsw,2);

	StampVoltageSource(VSOURCE(vsw)->v, k,j, VSOURCE(vsw)->vIdx, dc);
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_VSweep *vsw = obj;

	VSOURCE(vsw)->v = vsw->v1;
	
	UpdateStamp(vsw,dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_VSweep *vsw = obj;
	Uint curCycle;

	curCycle = dc->Telapsed / vsw->t;
	if (vsw->count != 0 && curCycle >= vsw->count)
		VSOURCE(vsw)->v = 0.0;
	else {
		/* fraction representing the progress in the current cycle */
		M_Real relProgress = dc->Telapsed / vsw->t - curCycle;
		VSOURCE(vsw)->v = vsw->v1 + (vsw->v2 - vsw->v1) * relProgress;
	}

	UpdateStamp(vsw,dc);
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
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_VSweep *vsw = p;

	vsw->v1 = M_ReadReal(buf);
	vsw->v2 = M_ReadReal(buf);
	vsw->t = M_ReadReal(buf);
	vsw->count = (int)AG_ReadUint8(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_VSweep *vsw = p;

	M_WriteReal(buf, vsw->v1);
	M_WriteReal(buf, vsw->v2);
	M_WriteReal(buf, vsw->t);
	AG_WriteUint8(buf, (Uint8)vsw->count);
	return (0);
}

static void *
Edit(void *p)
{
	ES_VSweep *vsw = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewReal(box, 0, "V", _("Start voltage: "), &vsw->v1);
	M_NumericalNewReal(box, 0, "V", _("End voltage: "), &vsw->v2);
	M_NumericalNewReal(box, 0, "s", _("Duration: "), &vsw->t);
	AG_NumericalNewInt(box, 0, NULL, _("Count (0=loop): "), &vsw->count);
	return (box);
}

ES_ComponentClass esVSweepClass = {
	{
		"ES_Component:ES_Vsource:ES_VSweep",
		sizeof(ES_VSweep),
		{ 0,0 },
		Init,
		NULL,		/* free_dataset */
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Voltage source (sweep)"),
	"Vsw",
	"Sources/Vsweep.eschem",
	"Generic|Sources",
	&esIconComponent,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
