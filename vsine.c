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

#include <eda.h>
#include "vsine.h"

const ES_Port esVSinePorts[] = {
	{  0, "" },
	{  1, "v+" },
	{  2, "v-" },
	{ -1 },
};

static void
IntStep(void *p, Uint ticks)
{
	ES_VSine *vs = p;

	VSOURCE(vs)->voltage = vs->vPeak*M_Sin(vs->phase);
	vs->phase += 1e-3*vs->f;
	if (vs->phase > M_PI*2) { vs->phase -= M_PI*2; }
}

static void
Init(void *p)
{
	ES_VSine *vs = p;

	ES_InitPorts(vs, esVSinePorts);
	vs->vPeak = 5.0;
	vs->f = 60.0;
	vs->phase = 0.0;
	COM(vs)->intStep = IntStep;
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
	AG_Window *win;
	AG_FSpinbutton *fsb;

	win = AG_WindowNew(0);
	fsb = AG_FSpinbuttonNew(win, 0, "V", _("Voltage (peak): "));
	AG_WidgetBind(fsb, "value", M_WIDGET_REAL, &vs->vPeak);
	fsb = AG_FSpinbuttonNew(win, 0, "Hz", _("Frequency: "));
	AG_WidgetBind(fsb, "value", M_WIDGET_REAL, &vs->f);
	return (win);
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
	N_("Sinusoidal voltage source"),
	"Vsin",
	"Sources/Vsine.eschem",
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
