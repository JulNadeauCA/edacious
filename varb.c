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
 * Arbitrary expression evaluator
 */

#include <eda.h>
#include "vsource.h"
#include "varb.h"
#include "interpreteur.h"


PARAM params[] = { {"pi" , M_PI},
		   {"t"  , 0.0}
};
const ES_Port esVArbPorts[] = {
	{  0, "" },
	{  1, "v+" },
	{  2, "v-" },
	{ -1 },
};

static void 
UpdateStamp(ES_VArb *vs,ES_SimDC *dc)
{
	Uint k = PNODE(vs,1);
	Uint j = PNODE(vs,2);

	StampVoltageSource(VSOURCE(vs)->voltage, k,j,
	    ES_VsourceName(vs),
	    dc->B, dc->C, dc->e);
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_VArb *vs = obj;
	
	/* Calculate initial voltage */
	params[1].Value = 0.0;
	Calculer(vs->exp, params, (sizeof(params)/sizeof(params[0])), &(VSOURCE(vs)->voltage));
	
	UpdateStamp(vs,dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_VArb *vs = obj;
	int ret;
	M_Real res;

	params[1].Value = dc->Telapsed;
	InterpreteurReset();
	ret = Calculer(vs->exp, params, (sizeof(params)/sizeof(params[0])), &res);
	if(ret != EVALUER_SUCCESS) {
		printf("Error !");
	}
	VSOURCE(vs)->voltage = res; 

	UpdateStamp(vs,dc);
}

static void
Init(void *p)
{
	ES_VArb *vs = p;

	ES_InitPorts(vs, esVArbPorts);

	strcpy(vs->exp, "sin(2*pi*t)");
	InterpreteurInit();
	
	COMPONENT(vs)->dcSimBegin = DC_SimBegin;
	COMPONENT(vs)->dcStepBegin = DC_StepBegin;
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_VArb *vs = p;

	AG_CopyString(vs->exp, buf, EXP_MAX_SIZE);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_VArb *vs = p;

	AG_WriteString(buf, vs->exp);
	return (0);
}

static void *
Edit(void *p)
{
	ES_VArb *vs = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	AG_LabelNewPolledMT(box, 0, &OBJECT(vs)->lock,
	    _("Effective voltage: %f"), &VSOURCE(vs)->voltage);
	AG_Textbox *tb = AG_TextboxNew(box, 0, "Expression : ");
	AG_TextboxBindASCII(tb, vs->exp, sizeof(vs->exp));

	return (box);
}

ES_ComponentClass esVArbClass = {
	{
		"ES_Component:ES_Vsource:ES_VArb",
		sizeof(ES_VArb),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Voltage source (expression)"),
	"Varb",
	"Sources/Varb.eschem",
	"Generic|Sources",
	&esIconVarb,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
