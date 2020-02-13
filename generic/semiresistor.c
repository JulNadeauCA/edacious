/*
 * Copyright (c) 2008 Antoine Levitt (smeuuh@gmail.com)
 * Copyright (c) 2008 Steven Herbst (herbst@mit.edu)
 * Copyright (c) 2005-2009 Julien Nadeau (vedge@hypertriton.com)
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

#include <core/core.h>
#include "generic.h"

const ES_Port esSemiResistorPorts[] = {
	{ 0, "" },
	{ 1, "A" },
	{ 2, "B" },
	{ -1 },
};

/* Compute the effective resistance from the parameters. */
static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_SemiResistor *r = obj;
	Uint k = PNODE(r,1);
	Uint j = PNODE(r,2);
	
	if (r->narrow >= r->l || r->narrow >= r->w) {
		AG_SetError("Narrowing is >= the length/width");
		return (-1);
	}
	r->rEff = r->rSh*(r->l - r->narrow)/(r->w - r->narrow);
	if (r->rEff <= M_MACHEP) {
		AG_SetError("Resistance is too small");
		return (-1);
	}

	InitStampConductance(k, j, r->s, dc);
	return (0);
}

/* TODO : move computations out of the loop */
static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_SemiResistor *r = obj;
	M_Real dT = dc->T0 - COMPONENT(r)->Tspec;
	M_Real g;

	g = 1.0/(r->rEff * (1.0 + r->Tc1*dT + r->Tc2*dT*dT));	
	StampConductance(g,r->s);
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_SemiResistor *r = obj;
	M_Real dT = dc->T0 - COMPONENT(r)->Tspec;
	M_Real g;

	g = 1.0/(r->rEff * (1.0 + r->Tc1*dT + r->Tc2*dT*dT));	
	StampConductance(g,r->s);
}

static void
Init(void *p)
{
	ES_SemiResistor *r = p;

	ES_InitPorts(r, esSemiResistorPorts);
	r->l = 2e-6;
	r->w = 1e-6;
	r->rSh = 1000.0;
	r->narrow = 0.0;
	r->Tc1 = 0.0;
	r->Tc2 = 0.0;
	r->rEff = 0.0;
	COMPONENT(r)->dcSimBegin = DC_SimBegin;
	COMPONENT(r)->dcStepBegin = DC_StepBegin;
	COMPONENT(r)->dcStepIter = DC_StepIter;

	M_BindReal(r, "l", &r->l);
	M_BindReal(r, "w", &r->w);
	M_BindReal(r, "rSh", &r->rSh);
	M_BindReal(r, "narrow", &r->narrow);
	M_BindReal(r, "Tc1", &r->Tc1);
	M_BindReal(r, "Tc2", &r->Tc2);
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
		           "(tc1=%g tc2=%g rsh=%g narrow=%g)\n",
		    nRmod, r->Tc1, r->Tc2, r->rSh, r->narrow);
		fprintf(f, "%s %d %d Rmod%u L=%g W=%g TEMP=%g\n",
		    OBJECT(r)->name, PNODE(r,1), PNODE(r,2),
		    nRmod, r->l, r->w, COMPONENT(r)->Tspec);
		nRmod++;
		break;
	}
	return (0);
}

static void *
Edit(void *p)
{
	ES_SemiResistor *r = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealPNZ(box, 0, "um", _("Length: "), &r->l);
	M_NumericalNewRealPNZ(box, 0, "um", _("Width: "), &r->w);
	M_NumericalNewRealPNZ(box, 0, "kohm", _("Resistance/sq: "), &r->rSh);
	M_NumericalNewRealP(box, 0, "um", _("Narrowing: "), &r->narrow);
	
	AG_SeparatorNewHoriz(box);
	
	M_NumericalNewRealP(box, 0, "mohm/degC", "Tc1: ", &r->Tc1);
	M_NumericalNewRealP(box, 0, "mohm/degC^2", "Tc2: ", &r->Tc2);

	if (COMCIRCUIT(r) != NULL) {
		AG_SeparatorNewHoriz(box);
		AG_LabelNewPolledMT(box, 0, &OBJECT(r)->lock,
		    "rEff: %[R]", &r->rEff);
	}
	return (box);
}

ES_ComponentClass esSemiResistorClass = {
	{
		"Edacious(Circuit:Component:SemiResistor)"
		"@generic",
		sizeof(ES_SemiResistor),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		Edit
	},
	N_("Resistor (semiconductor)"),
	"R",
	"Generic|Resistances|Semiconductor",
	&esIconResistor,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	Export,
	NULL			/* connect */
};
