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
#include "vsquare.h"

const AG_Version esVSquareVer = {
	"agar-eda square voltage source",
	0, 0
};

const ES_ComponentOps esVSquareOps = {
	{
		ES_VSquareInit,
		ES_VsourceReinit,
		ES_VsourceDestroy,
		ES_VSquareLoad,
		ES_VSquareSave,
		ES_ComponentEdit
	},
	N_("Square voltage source"),
	"Vsq",
	ES_VSquareDraw,
	ES_VSquareEdit,
	NULL,			/* menu */
	NULL,			/* connect */
	NULL,			/* disconnect */
	ES_VSquareExport
};

const ES_Port esVSquarePorts[] = {
	{ 0, "",  0.0, 0.0 },
	{ 1, "v+", 0.0, 0.0 },
	{ 2, "v-", 0.0, 2.0 },
	{ -1 },
};

enum {
	VSOURCE_SYM_LINEAR,
	VSOURCE_SYM_CIRCULAR
} esVSquareStyle = 1;

void
ES_VSquareDraw(void *p, VG *vg)
{
	ES_VSquare *vs = p;
	
	switch (esVSquareStyle) {
	case VSOURCE_SYM_LINEAR:
		VG_Begin(vg, VG_LINES);
		VG_Vertex2(vg, 0.0000, 0.0000);
		VG_Vertex2(vg, 0.0000, 0.3125);
		VG_Vertex2(vg, 0.0000, 0.6875);
		VG_Vertex2(vg, 0.0000, 1.0000);
		VG_Vertex2(vg, -0.0937, 0.3125);
		VG_Vertex2(vg, +0.0937, 0.3125);
		VG_Vertex2(vg, -0.1875, 0.4375);
		VG_Vertex2(vg, +0.1875, 0.4375);
		VG_Vertex2(vg, -0.0937, 0.5625);
		VG_Vertex2(vg, +0.0937, 0.5625);
		VG_Vertex2(vg, -0.1875, 0.6875);
		VG_Vertex2(vg, +0.1875, 0.6875);
		VG_End(vg);

		VG_Begin(vg, VG_TEXT);
		VG_SetStyle(vg, "component-name");
		VG_Vertex2(vg, -0.0468, 0.0625);
		VG_TextAlignment(vg, VG_ALIGN_TR);
		VG_Printf(vg, "%s\n%.2f/%.2fV\n", AGOBJECT(vs)->name,
		    vs->vL, vs->vH);
		VG_End(vg);
		break;
	case VSOURCE_SYM_CIRCULAR:
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
		VG_Printf(vg, "%s (%.2f/%.2f)", AGOBJECT(vs)->name,
		    vs->vL, vs->vH);
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
		break;
	}
}

void
ES_VSquareInit(void *p, const char *name)
{
	ES_VSquare *vs = p;

	ES_VsourceInit(vs, name);
	AG_ObjectSetType(vs, "component.vsource.square");
	AG_ObjectSetOps(vs, &esVSquareOps);
	ES_ComponentSetPorts(vs, esVSquarePorts);
	vs->vH = 5.0;
	vs->vL = 0.0;
	vs->tH = 1000;
	vs->tL = 1000;
	vs->t = 0;
}

int
ES_VSquareLoad(void *p, AG_Netbuf *buf)
{
	ES_VSquare *vs = p;

	if (AG_ReadVersion(buf, &esVSquareVer, NULL) == -1 ||
	    ES_ComponentLoad(vs, buf) == -1)
		return (-1);

	vs->vH = SC_ReadReal(buf);
	vs->vL = SC_ReadReal(buf);
	vs->tH = SC_ReadQTime(buf);
	vs->tL = SC_ReadQTime(buf);
	return (0);
}

int
ES_VSquareSave(void *p, AG_Netbuf *buf)
{
	ES_VSquare *vs = p;

	AG_WriteVersion(buf, &esVSquareVer);
	if (ES_ComponentSave(vs, buf) == -1)
		return (-1);

	SC_WriteReal(buf, vs->vH);
	SC_WriteReal(buf, vs->vL);
	SC_WriteQTime(buf, vs->tH);
	SC_WriteQTime(buf, vs->tL);
	return (0);
}

int
ES_VSquareExport(void *p, enum circuit_format fmt, FILE *f)
{
	ES_VSquare *vs = p;

	/* TODO */
	return (0);
}

#ifdef EDITION
void *
ES_VSquareEdit(void *p)
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
	AG_WidgetBind(fsb, "value", SC_WIDGET_QTIME, &vs->tH);
	return (win);
}
#endif /* EDITION */
