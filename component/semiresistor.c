/*
 * Copyright (c) 2004, 2005 Hypertriton, Inc. <http://hypertriton.com/>
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
#include "semiresistor.h"

const ES_Port esSemiResistorPinout[] = {
	{ 0, "",  0.000, 0.625 },
	{ 1, "A", 0.000, 0.000 },
	{ 2, "B", 1.250, 0.000 },
	{ -1 },
};

static void
Draw(void *p, VG *vg)
{
	ES_SemiResistor *r = p;

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

	VG_Begin(vg, VG_TEXT);
	VG_SetStyle(vg, "component-name");
	VG_Vertex2(vg, 0.625, 0.000);
	VG_TextAlignment(vg, VG_ALIGN_MC);

	VG_Printf(vg, "%s", AGOBJECT(r)->name);
	VG_End(vg);
}

static void
Init(void *p)
{
	ES_SemiResistor *r = p;

	ES_ComponentSetPorts(r, esSemiResistorPinout);
	r->l = 2e-6;
	r->w = 1e-6;
	r->rsh = 1000;
	r->defw = 1e-6;
	r->narrow = 0;
	r->Tc1 = 0.0;
	r->Tc2 = 0.0;
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_SemiResistor *r = p;

	r->l = AG_ReadDouble(buf);
	r->w = AG_ReadDouble(buf);
	r->rsh = AG_ReadDouble(buf);
	r->defw = AG_ReadDouble(buf);
	r->narrow = AG_ReadDouble(buf);
	r->Tc1 = AG_ReadFloat(buf);
	r->Tc2 = AG_ReadFloat(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_SemiResistor *r = p;

	AG_WriteDouble(buf, r->l);
	AG_WriteDouble(buf, r->w);
	AG_WriteDouble(buf, r->rsh);
	AG_WriteDouble(buf, r->defw);
	AG_WriteDouble(buf, r->narrow);
	AG_WriteFloat(buf, r->Tc1);
	AG_WriteFloat(buf, r->Tc2);
	return (0);
}

static int
Export(void *p, enum circuit_format fmt, FILE *f)
{
	ES_SemiResistor *r = p;
	static int nRmod = 1;

	if (PNODE(r,1) == -1 ||
	    PNODE(r,2) == -1)
		return (0);
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		fprintf(f, ".MODEL Rmod%u R "
		           "(tc1=%g tc2=%g rsh=%g defw=%g narrow=%g "
			   "tnom=%g)\n",
		    nRmod, r->Tc1, r->Tc2, r->rsh, r->defw, r->narrow,
		    COM_T0);
		fprintf(f, "%s %d %d Rmod%u L=%g W=%g TEMP=%g\n",
		    AGOBJECT(r)->name, PNODE(r,1), PNODE(r,2),
		    nRmod, r->l, r->w, COM(r)->Tspec);
		nRmod++;
		break;
	}
	return (0);
}

#if 0
static double
Resistance(void *p, ES_Port *p1, ES_Port *p2)
{
	ES_SemiResistor *r = p;
	double deltaT = COM_T0 - COM(r)->Tspec;
	double R;

	R = r->rsh*(r->l - r->narrow)/(r->w - r->narrow);
	return (R*(1.0 + r->Tc1*deltaT + r->Tc2*deltaT*deltaT));
}
#endif

static void *
Edit(void *p)
{
	ES_SemiResistor *r = p;
	AG_Window *win, *subwin;
	AG_Spinbutton *sb;
	AG_FSpinbutton *fsb;
	AG_MFSpinbutton *mfsb;

	win = AG_WindowNew(0);

	mfsb = AG_MFSpinbuttonNew(win, 0, "um", "x", _("Geometry (LxW): "));
	AG_WidgetBind(mfsb, "xvalue", AG_WIDGET_DOUBLE, &r->l);
	AG_WidgetBind(mfsb, "yvalue", AG_WIDGET_DOUBLE, &r->w);
	AG_MFSpinbuttonSetMin(mfsb, 1e-6);
	
	fsb = AG_FSpinbuttonNew(win, 0, "kohm", _("Sheet resistance/sq: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &r->rsh);
	AG_FSpinbuttonSetMin(fsb, 0);
	
	fsb = AG_FSpinbuttonNew(win, 0, "um",
	    _("Narrowing due to side etching: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &r->narrow);
	AG_FSpinbuttonSetMin(fsb, 0);
	
	fsb = AG_FSpinbuttonNew(win, 0, "mohm/degC", _("Temp. coefficient: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &r->Tc1);
	fsb = AG_FSpinbuttonNew(win, 0, "mohm/degC^2", _("Temp. coefficient"));
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &r->Tc2);
	return (win);
}

ES_ComponentClass esSemiResistorClass = {
	{
		"ES_Component:ES_SemiResistor",
		sizeof(ES_SemiResistor),
		{ 0,0 },
		Init,
		NULL,			/* reinit */
		NULL,			/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Semiconductor resistor"),
	"R",
	Draw,
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	Export,
	NULL			/* connect */
};
