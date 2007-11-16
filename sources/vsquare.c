/*
 * Copyright (c) 2005 Hypertriton, Inc. <http://hypertriton.com/>
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

#include <agar/core.h>
#include <agar/vg.h>
#include <agar/gui.h>

#include "eda.h"
#include "vsquare.h"

const ES_Port esVSquarePorts[] = {
	{  0, "",   0.0, 0.0 },
	{  1, "v+", 0.0, 0.0 },
	{  2, "v-", 0.0, 2.0 },
	{ -1 },
};

static void
Draw(void *p, VG *vg)
{
	ES_VSquare *vs = p;
	
	VG_Begin(vg, VG_LINES);
	VG_Vertex2(vg, 0.000, 0.000);
	VG_Vertex2(vg, 0.000, 0.400);
	VG_Vertex2(vg, 0.000, 1.600);
	VG_Vertex2(vg, 0.000, 2.000);
	VG_End(vg);

	VG_Begin(vg, VG_CIRCLE);
	VG_Vertex2(vg, 0.0, 1.0);
	VG_CircleRadius(vg, 0.6);
	VG_End(vg);

	VG_Begin(vg, VG_TEXT);
	VG_SetStyle(vg, "component-name");
	VG_Vertex2(vg, 0.0, 1.0);
	VG_TextAlignment(vg, VG_ALIGN_MC);
	VG_Printf(vg, "%s", AGOBJECT(vs)->name);
	VG_End(vg);
		
	VG_Begin(vg, VG_TEXT);
	VG_SetStyle(vg, "component-name");
	VG_Vertex2(vg, 0.0, 0.6);
	VG_TextAlignment(vg, VG_ALIGN_MC);
	VG_Printf(vg, "+");
	VG_End(vg);
		
	VG_Begin(vg, VG_TEXT);
	VG_SetStyle(vg, "component-name");
	VG_Vertex2(vg, 0.0, 1.4);
	VG_TextAlignment(vg, VG_ALIGN_MC);
	VG_Printf(vg, "-");
	VG_End(vg);
}

static void
Init(void *p)
{
	ES_VSquare *vs = p;

	ES_ComponentSetPorts(vs, esVSquarePorts);
	vs->vH = 5.0;
	vs->vL = 0.0;
	vs->t = 0;
	vs->tH = 100;
	vs->tL = 100;
	COM(vs)->intStep = ES_VSquareStep;
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_VSquare *vs = p;

	vs->vH = SC_ReadReal(buf);
	vs->vL = SC_ReadReal(buf);
	vs->tH = SC_ReadQTime(buf);
	vs->tL = SC_ReadQTime(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_VSquare *vs = p;

	SC_WriteReal(buf, vs->vH);
	SC_WriteReal(buf, vs->vL);
	SC_WriteQTime(buf, vs->tH);
	SC_WriteQTime(buf, vs->tL);
	return (0);
}

static int
Export(void *p, enum circuit_format fmt, FILE *f)
{
	ES_VSquare *vs = p;

	/* TODO */
	return (0);
}

void
ES_VSquareStep(void *p, Uint ticks)
{
	ES_VSquare *vs = p;

	if (VSOURCE(vs)->voltage == vs->vH && ++vs->t > vs->tH) {
		VSOURCE(vs)->voltage = vs->vL;
		vs->t = 0;
	} else if (VSOURCE(vs)->voltage == vs->vL && ++vs->t > vs->tL) {
		VSOURCE(vs)->voltage = vs->vH;
		vs->t = 0;
	} else if (VSOURCE(vs)->voltage != vs->vL &&
	           VSOURCE(vs)->voltage != vs->vH) {
		VSOURCE(vs)->voltage = vs->vL;
	}
}

static void *
Edit(void *p)
{
	ES_VSquare *vs = p;
	AG_Window *win;
	AG_FSpinbutton *fsb;

	win = AG_WindowNew(0);
	fsb = AG_FSpinbuttonNew(win, 0, "V", _("Voltage (high): "));
	AG_WidgetBind(fsb, "value", SC_WIDGET_REAL, &vs->vH);
	fsb = AG_FSpinbuttonNew(win, 0, "V", _("Voltage (low): "));
	AG_WidgetBind(fsb, "value", SC_WIDGET_REAL, &vs->vL);
	fsb = AG_FSpinbuttonNew(win, 0, "ns", _("Time (high): "));
	AG_WidgetBind(fsb, "value", SC_WIDGET_QTIME, &vs->tH);
	fsb = AG_FSpinbuttonNew(win, 0, "ns", _("Time (low): "));
	AG_WidgetBind(fsb, "value", SC_WIDGET_QTIME, &vs->tL);
	return (win);
}

const ES_ComponentClass esVSquareClass = {
	{
		"ES_Component:ES_Vsource:ES_VSquare",
		sizeof(ES_VSquare),
		{ 0,0 },
		Init,
		NULL,		/* free_dataset */
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Square voltage source"),
	"Vsq",
	Draw,
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	Export,
	NULL			/* connect */
};
