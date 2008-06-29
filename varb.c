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
UpdateStamp(ES_VArb *va, ES_SimDC *dc)
{
	Uint k = PNODE(va,1);
	Uint j = PNODE(va,2);

	StampVoltageSource(VSOURCE(va)->v, k,j, VSOURCE(va)->vIdx,
	    dc->B, dc->C, dc->e);
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_VArb *va = obj;
	
	/* Calculate initial voltage */
	params[1].Value = 0.0;
	Calculer(va->exp, params, (sizeof(params)/sizeof(params[0])),
	    &(VSOURCE(va)->v));
	
	UpdateStamp(va,dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_VArb *va = obj;
	int ret;
	M_Real res;

	params[1].Value = dc->Telapsed;
	InterpreteurReset();
	ret = Calculer(va->exp, params, (sizeof(params)/sizeof(params[0])),
	    &res);
	if (ret != EVALUER_SUCCESS) {
		printf("Error !");
	}
	VSOURCE(va)->v = res; 

	UpdateStamp(va,dc);
}

static void
Init(void *p)
{
	ES_VArb *va = p;

	ES_InitPorts(va, esVArbPorts);

	Strlcpy(va->exp, "sin(2*pi*t)", sizeof(va->exp));
	va->flags = 0;
	InterpreteurInit();
	
	COMPONENT(va)->dcSimBegin = DC_SimBegin;
	COMPONENT(va)->dcStepBegin = DC_StepBegin;
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_VArb *va = p;

	AG_CopyString(va->exp, buf, sizeof(va->exp));
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_VArb *va = p;

	AG_WriteString(buf, va->exp);
	return (0);
}

static void
UpdatePlot(AG_Event *event)
{
	M_Plot *pl = AG_PTR(1);
	ES_VArb *va = AG_PTR(2);
	M_Real i, v = 0.0;
	int ret;

	M_PlotClear(pl);
	pl->flags &= ~(ES_VARB_ERROR);
	for (i = 0.0; i < 1.5; i += 0.01) {
		params[1].Value = i;
		InterpreteurReset();
		ret = Calculer(va->exp, params,
		    (sizeof(params)/sizeof(params[0])), &v);
		if (ret != EVALUER_SUCCESS) {
			va->flags |= ES_VARB_ERROR;
			return;
		}
		M_PlotReal(pl, v);
#ifdef THREADS
		SDL_Delay(5);
#endif
	}
}

static void *
Edit(void *p)
{
	ES_VArb *va = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);
	M_Plotter *ptr;
	M_Plot *pl;
	AG_Textbox *tb;
	AG_Event *ev;

	AG_LabelNewPolledMT(box, 0, &OBJECT(va)->lock,
	    _("Effective voltage: %fv"), &VSOURCE(va)->v);
	
	tb = AG_TextboxNew(box, 0, "v(t) = ");
	AG_TextboxBindASCII(tb, va->exp, sizeof(va->exp));
	AG_SeparatorNewHoriz(box);

	ptr = M_PlotterNew(box, M_PLOTTER_EXPAND);
	M_PlotterSizeHint(ptr, 100, 50);
	pl = M_PlotNew(ptr, M_PLOT_LINEAR);
	M_PlotSetLabel(pl, "v(t)");
	M_PlotSetScale(pl, 0.0, 16.0);
	ev = AG_SetEvent(tb, "textbox-return", UpdatePlot, "%p,%p", pl, va);
#ifdef THREADS
	ev->flags |= AG_EVENT_ASYNC;
#endif
	AG_PostEvent(NULL, tb, "textbox-return", NULL);

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
