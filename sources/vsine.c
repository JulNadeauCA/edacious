/*	$Csoft: vsource.c,v 1.5 2005/09/27 03:34:09 vedge Exp $	*/

/*
 * Copyright (c) 2005 CubeSoft Communications, Inc.
 * <http://www.winds-triton.com>
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
#include "vsine.h"

const AG_Version esVSineVer = {
	"agar-eda sinusoidal voltage source",
	0, 0
};

const ES_ComponentOps esVSineOps = {
	{
		ES_VSineInit,
		ES_VsourceReinit,
		ES_VsourceDestroy,
		ES_VSineLoad,
		ES_VSineSave,
		ES_ComponentEdit
	},
	N_("Sinusoidal voltage source"),
	"Vsin",
	ES_VSineDraw,
	ES_VSineEdit,
	NULL,			/* menu */
	NULL,			/* connect */
	NULL,			/* disconnect */
	NULL,			/* export */
};

const ES_Port esVSinePorts[] = {
	{  0, "",   0.0, 0.0 },
	{  1, "v+", 0.0, 0.0 },
	{  2, "v-", 0.0, 2.0 },
	{ -1 },
};

void
ES_VSineDraw(void *p, VG *vg)
{
	ES_VSine *vs = p;
	
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

void
ES_VSineInit(void *p, const char *name)
{
	ES_VSine *vs = p;

	ES_VsourceInit(vs, name);
	ES_ComponentSetType(vs, "component.vsource.sine");
	ES_ComponentSetOps(vs, &esVSineOps);
	ES_ComponentSetPorts(vs, esVSinePorts);
	vs->vPeak = 5.0;
	vs->f = 60.0;
	vs->phase = 0.0;

	COM(vs)->intStep = ES_VSineStep;
	COM(vs)->intUpdate = ES_VSineUpdate;
}

int
ES_VSineLoad(void *p, AG_Netbuf *buf)
{
	ES_VSine *vs = p;

	if (AG_ReadVersion(buf, &esVSineVer, NULL) == -1 ||
	    ES_VsourceLoad(vs, buf) == -1)
		return (-1);

	vs->vPeak = SC_ReadReal(buf);
	vs->f = SC_ReadReal(buf);
	return (0);
}

int
ES_VSineSave(void *p, AG_Netbuf *buf)
{
	ES_VSine *vs = p;

	AG_WriteVersion(buf, &esVSineVer);
	if (ES_VsourceSave(vs, buf) == -1)
		return (-1);

	SC_WriteReal(buf, vs->vPeak);
	SC_WriteReal(buf, vs->f);
	return (0);
}

void
ES_VSineStep(void *p, Uint ticks)
{
	ES_VSine *vs = p;

	VSOURCE(vs)->voltage = vs->vPeak*SC_Sin(vs->phase);
	vs->phase += 1e-3*vs->f;
	if (vs->phase > 1.0) { vs->phase -= 1.0; }
}

void
ES_VSineUpdate(void *p)
{
	ES_VSine *vs = p;

}

#ifdef EDITION
void *
ES_VSineEdit(void *p)
{
	ES_VSine *vs = p;
	AG_Window *win;
	AG_FSpinbutton *fsb;

	win = AG_WindowNew(0);
	fsb = AG_FSpinbuttonNew(win, 0, "V", _("Voltage (peak): "));
	AG_WidgetBind(fsb, "value", SC_WIDGET_REAL, &vs->vPeak);
	fsb = AG_FSpinbuttonNew(win, 0, "Hz", _("Frequency: "));
	AG_WidgetBind(fsb, "value", SC_WIDGET_REAL, &vs->f);
	return (win);
}
#endif /* EDITION */