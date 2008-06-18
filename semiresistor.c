/*
 * Copyright (c) 2004-2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Model for the semiconductor resistor.
 */

#include <eda.h>
#include "semiresistor.h"

const ES_Port esSemiResistorPorts[] = {
	{ 0, "" },
	{ 1, "A" },
	{ 2, "B" },
	{ -1 },
};

static void
Init(void *p)
{
	ES_SemiResistor *r = p;

	ES_InitPorts(r, esSemiResistorPorts);
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

	r->l = M_ReadReal(buf);
	r->w = M_ReadReal(buf);
	r->rsh = M_ReadReal(buf);
	r->defw = M_ReadReal(buf);
	r->narrow = M_ReadReal(buf);
	r->Tc1 = M_ReadReal(buf);
	r->Tc2 = M_ReadReal(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_SemiResistor *r = p;

	M_WriteReal(buf, r->l);
	M_WriteReal(buf, r->w);
	M_WriteReal(buf, r->rsh);
	M_WriteReal(buf, r->defw);
	M_WriteReal(buf, r->narrow);
	M_WriteReal(buf, r->Tc1);
	M_WriteReal(buf, r->Tc2);
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
		    COMCIRCUIT(r)->T0);
		fprintf(f, "%s %d %d Rmod%u L=%g W=%g TEMP=%g\n",
		    OBJECT(r)->name, PNODE(r,1), PNODE(r,2),
		    nRmod, r->l, r->w, COMPONENT(r)->Tspec);
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
	double deltaT = COMCIRCUIT(r)->T0 - COMPONENT(r)->Tspec;
	double R;

	R = r->rsh*(r->l - r->narrow)/(r->w - r->narrow);
	return (R*(1.0 + r->Tc1*deltaT + r->Tc2*deltaT*deltaT));
}
#endif

static void *
Edit(void *p)
{
	ES_SemiResistor *r = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);
	AG_MFSpinbutton *mfsb;

	mfsb = AG_MFSpinbuttonNew(box, 0, "um", "x", _("Geometry (LxW): "));
	M_WidgetBindReal(mfsb, "xvalue", &r->l);
	M_WidgetBindReal(mfsb, "yvalue", &r->w);
	AG_MFSpinbuttonSetMin(mfsb, M_TINYVAL);
	
	M_NumericalNewRealR(box, 0, "kohm", _("Sheet resistance/sq: "),
	    &r->rsh, M_TINYVAL, HUGE_VAL);
	M_NumericalNewReal(box, 0, "um", _("Default width: "),
	    &r->defw);
	M_NumericalNewReal(box, 0, "um", _("Narrowing due to side etching: "),
	    &r->narrow);
	M_NumericalNewReal(box, 0, "mohm/degC", _("1st order temp. coeff: "),
	    &r->Tc1);
	M_NumericalNewReal(box, 0, "mohm/degC^2", _("2nd order temp. coeff: "),
	    &r->Tc2);

	return (box);
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
	N_("Resistor (semiconductor)"),
	"R",
	"Resistor.eschem",
	"Generic|Resistances|Semiconductor",
	&esIconResistor,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	Export,
	NULL			/* connect */
};
