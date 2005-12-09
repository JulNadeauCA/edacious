/*	$Csoft: resistor.c,v 1.5 2005/09/27 03:34:09 vedge Exp $	*/

/*
 * Copyright (c) 2004, 2005 CubeSoft Communications, Inc.
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
#include "resistor.h"

const AG_Version esResistorVer = {
	"agar-eda resistor",
	0, 0
};

const ES_ComponentOps esResistorOps = {
	{
		ES_ResistorInit,
		NULL,			/* reinit */
		ES_ComponentDestroy,
		ES_ResistorLoad,
		ES_ResistorSave,
		ES_ComponentEdit
	},
	N_("Resistor"),
	"R",
	ES_ResistorDraw,
	ES_ResistorEdit,
	NULL,
	ES_ResistorExport,
	NULL,			/* tick */
	ES_ResistorResistance,
	NULL,			/* capacitance */
	NULL,			/* inductance */
	NULL			/* isource */
};

const ES_Port esResistorPinout[] = {
	{ 0, "",  0.000, 0.625 },
	{ 1, "A", 0.000, 0.000 },
	{ 2, "B", 1.250, 0.000 },
	{ -1 },
};

int esResistorDispVal = 0;
enum {
	RESISTOR_BOX_STYLE,
	RESISTOR_LINE_STYLE
} esResistorStyle = 0;

void
ES_ResistorDraw(void *p, VG *vg)
{
	ES_Resistor *r = p;

	switch (esResistorStyle) {
	case RESISTOR_BOX_STYLE:
		VG_Begin(vg, VG_LINES);
		VG_Vertex2(vg, 0.000, 0.000);
		VG_Vertex2(vg, 0.156, 0.000);
		VG_Vertex2(vg, 1.250, 0.000);
		VG_Vertex2(vg, 1.09375, 0.000);
		VG_End(vg);
		VG_Begin(vg, VG_LINE_LOOP);
		VG_Vertex2(vg, 0.156, -0.240);
		VG_Vertex2(vg, 0.156,  0.240);
		VG_Vertex2(vg, 1.09375,  0.240);
		VG_Vertex2(vg, 1.09375, -0.240);
		VG_End(vg);
		break;
	case RESISTOR_LINE_STYLE:
		VG_Begin(vg, VG_LINE_STRIP);
		VG_Vertex2(vg, 0.000, 0.125);
		VG_Vertex2(vg, 0.125, 0.000);
		VG_Vertex2(vg, 0.250, 0.125);
		VG_Vertex2(vg, 0.375, 0.000);
		VG_Vertex2(vg, 0.500, 0.125);
		VG_Vertex2(vg, 0.625, 0.000);
		VG_Vertex2(vg, 0.750, 0.125);
		VG_Vertex2(vg, 0.875, 0.000);
		VG_Vertex2(vg, 1.000, 0.000);
		VG_End(vg);
		break;
	}

	VG_Begin(vg, VG_TEXT);
	VG_SetStyle(vg, "component-name");
	VG_Vertex2(vg, 0.625, 0.000);
	VG_TextAlignment(vg, VG_ALIGN_MC);

	if (esResistorDispVal) {
		VG_Printf(vg, "%s (%.2f\xce\xa9)", AGOBJECT(r)->name,
		    r->resistance);
	} else {
		VG_Printf(vg, "%s", AGOBJECT(r)->name);
	}
	VG_End(vg);
}

void
ES_ResistorInit(void *p, const char *name)
{
	ES_Resistor *r = p;

	ES_ComponentInit(r, "resistor", name, &esResistorOps, esResistorPinout);
	r->resistance = 1;
	r->power_rating = HUGE_VAL;
	r->tolerance = 0;
}

int
ES_ResistorLoad(void *p, AG_Netbuf *buf)
{
	ES_Resistor *r = p;

	if (AG_ReadVersion(buf, &esResistorVer, NULL) == -1 ||
	    ES_ComponentLoad(r, buf) == -1)
		return (-1);

	r->resistance = AG_ReadDouble(buf);
	r->tolerance = (int)AG_ReadUint32(buf);
	r->power_rating = AG_ReadDouble(buf);
	r->Tc1 = AG_ReadFloat(buf);
	r->Tc2 = AG_ReadFloat(buf);
	return (0);
}

int
ES_ResistorSave(void *p, AG_Netbuf *buf)
{
	ES_Resistor *r = p;

	AG_WriteVersion(buf, &esResistorVer);
	if (ES_ComponentSave(r, buf) == -1)
		return (-1);

	AG_WriteDouble(buf, r->resistance);
	AG_WriteUint32(buf, (Uint32)r->tolerance);
	AG_WriteDouble(buf, r->power_rating);
	AG_WriteFloat(buf, r->Tc1);
	AG_WriteFloat(buf, r->Tc2);
	return (0);
}

int
ES_ResistorExport(void *p, enum circuit_format fmt, FILE *f)
{
	ES_Resistor *r = p;

	if (PNODE(r,1) == -1 ||
	    PNODE(r,2) == -1)
		return (0);
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		fprintf(f, "%s %d %d %g\n", AGOBJECT(r)->name,
		    PNODE(r,1), PNODE(r,2), r->resistance);
		break;
	}
	return (0);
}

double
ES_ResistorResistance(void *p, ES_Port *p1, ES_Port *p2)
{
	ES_Resistor *r = p;
	double deltaT = COM(r)->Tnom - COM(r)->Tspec;

	return (r->resistance * (1.0 + r->Tc1*deltaT + r->Tc2*deltaT*deltaT));
}

#ifdef EDITION
AG_Window *
ES_ResistorEdit(void *p)
{
	ES_Resistor *r = p;
	AG_Window *win;
	AG_Spinbutton *sb;
	AG_FSpinbutton *fsb;

	win = AG_WindowNew(0);

	fsb = AG_FSpinbuttonNew(win, 0, "ohms", _("Resistance: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &r->resistance);
	AG_FSpinbuttonSetMin(fsb, 1.0);
	
	sb = AG_SpinbuttonNew(win, 0, _("Tolerance (%%): "));
	AG_WidgetBind(sb, "value", AG_WIDGET_INT, &r->tolerance);
	AG_SpinbuttonSetRange(sb, 1, 100);

	fsb = AG_FSpinbuttonNew(win, 0, "W", _("Power rating: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &r->power_rating);
	AG_FSpinbuttonSetMin(fsb, 0);
	
	fsb = AG_FSpinbuttonNew(win, 0, "mohms/degC", _("Temp. coefficient: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &r->Tc1);
	fsb = AG_FSpinbuttonNew(win, 0, "mohms/degC^2",
	    _("Temp. coefficient: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &r->Tc2);

	return (win);
}
#endif /* EDITION */
