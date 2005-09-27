/*	$Csoft: resistor.c,v 1.4 2005/09/15 02:04:49 vedge Exp $	*/

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

#include <engine/engine.h>
#include <engine/vg/vg.h>

#ifdef EDITION
#include <engine/widget/window.h>
#include <engine/widget/spinbutton.h>
#include <engine/widget/fspinbutton.h>
#endif

#include "resistor.h"

const AG_Version resistor_ver = {
	"agar-eda resistor",
	0, 0
};

const struct component_ops resistor_ops = {
	{
		resistor_init,
		NULL,			/* reinit */
		component_destroy,
		resistor_load,
		resistor_save,
		component_edit
	},
	N_("Resistor"),
	"R",
	resistor_draw,
	resistor_edit,
	NULL,
	resistor_export,
	NULL,			/* tick */
	resistor_resistance,
	NULL,			/* capacitance */
	NULL,			/* inductance */
	NULL			/* isource */
};

const struct pin resistor_pinout[] = {
	{ 0, "",  0.000, 0.625 },
	{ 1, "A", 0.000, 0.000 },
	{ 2, "B", 1.250, 0.000 },
	{ -1 },
};

enum {
	RESISTOR_BOX_STYLE,
	RESISTOR_LINE_STYLE
} resistor_style = 0;

int resistor_dispval = 0;

void
resistor_draw(void *p, VG *vg)
{
	struct resistor *r = p;

	switch (resistor_style) {
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

	if (resistor_dispval) {
		VG_Printf(vg, "%s\n%.2f \xce\xa9\n", AGOBJECT(r)->name,
		    r->resistance);
	} else {
		VG_Printf(vg, "%s\n", AGOBJECT(r)->name);
	}
	VG_End(vg);
}

void
resistor_init(void *p, const char *name)
{
	struct resistor *r = p;

	component_init(r, "resistor", name, &resistor_ops, resistor_pinout);
	r->resistance = 1;
	r->power_rating = HUGE_VAL;
	r->tolerance = 0;
}

int
resistor_load(void *p, AG_Netbuf *buf)
{
	struct resistor *r = p;

	if (AG_ReadVersion(buf, &resistor_ver, NULL) == -1 ||
	    component_load(r, buf) == -1)
		return (-1);

	r->resistance = AG_ReadDouble(buf);
	r->tolerance = (int)AG_ReadUint32(buf);
	r->power_rating = AG_ReadDouble(buf);
	r->Tc1 = AG_ReadFloat(buf);
	r->Tc2 = AG_ReadFloat(buf);
	return (0);
}

int
resistor_save(void *p, AG_Netbuf *buf)
{
	struct resistor *r = p;

	AG_WriteVersion(buf, &resistor_ver);
	if (component_save(r, buf) == -1)
		return (-1);

	AG_WriteDouble(buf, r->resistance);
	AG_WriteUint32(buf, (Uint32)r->tolerance);
	AG_WriteDouble(buf, r->power_rating);
	AG_WriteFloat(buf, r->Tc1);
	AG_WriteFloat(buf, r->Tc2);
	return (0);
}

int
resistor_export(void *p, enum circuit_format fmt, FILE *f)
{
	struct resistor *r = p;

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
resistor_resistance(void *p, struct pin *p1, struct pin *p2)
{
	struct resistor *r = p;
	double deltaT = COM(r)->Tnom - COM(r)->Tspec;

	return (r->resistance * (1.0 + r->Tc1*deltaT + r->Tc2*deltaT*deltaT));
}

#ifdef EDITION
AG_Window *
resistor_edit(void *p)
{
	struct resistor *r = p;
	AG_Window *win;
	AG_Spinbutton *sb;
	AG_FSpinbutton *fsb;

	win = AG_WindowNew(0, NULL);

	fsb = AG_FSpinbuttonNew(win, "ohms", _("Resistance: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &r->resistance);
	AG_FSpinbuttonSetMin(fsb, 1.0);
	
	sb = AG_SpinbuttonNew(win, _("Tolerance (%%): "));
	AG_WidgetBind(sb, "value", AG_WIDGET_INT, &r->tolerance);
	AG_SpinbuttonSetRange(sb, 1, 100);

	fsb = AG_FSpinbuttonNew(win, "W", _("Power rating: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &r->power_rating);
	AG_FSpinbuttonSetMin(fsb, 0);
	
	fsb = AG_FSpinbuttonNew(win, "mohms/degC", _("Temp. coefficient: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &r->Tc1);
	fsb = AG_FSpinbuttonNew(win, "mohms/degC^2", _("Temp. coefficient: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &r->Tc2);

	return (win);
}
#endif /* EDITION */
