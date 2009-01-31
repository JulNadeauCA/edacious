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
 * Model for the resistor.
 */

#include <core/core.h>
#include "generic.h"

const ES_Port esResistorPorts[] = {
	{ 0, "" },
	{ 1, "A" },
	{ 2, "B" },
	{ -1 },
};

static int
Export(void *p, enum circuit_format fmt, FILE *f)
{
	ES_Resistor *r = p;

	if (PNODE(r,1) == -1 ||
	    PNODE(r,2) == -1)
		return (0);
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		fprintf(f, "%s %d %d %g\n", OBJECT(r)->name,
		    PNODE(r,1), PNODE(r,2), r->R);
		break;
	}
	return (0);
}

static void
UpdateModel(ES_Resistor *r, ES_SimDC *dc)
{
	M_Real dT = dc->T0 - COMPONENT(r)->Tspec;

	r->g = 1.0/(r->R*(1.0 + r->Tc1*dT + r->Tc2*dT*dT));
}

static __inline__ void
Stamp(ES_Resistor *r, ES_SimDC *dc)
{
	StampConductance(r->g, r->s);
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_Resistor *r = obj;
	Uint k = PNODE(r,1);
	Uint j = PNODE(r,2);

	if (r->R == 0.0 || k == -1 || j == -1 || (k == 0 && j == 0)) {
		AG_SetError("Null resistance");
		return (-1);
	}
	InitStampConductance(k, j, r->s, dc);
	UpdateModel(r, dc);
	Stamp(r, dc);
	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_Resistor *r = obj;

	Stamp(r, dc);
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_Resistor *r = obj;

	Stamp(r, dc);
}

static void
Init(void *p)
{
	ES_Resistor *r = p;

	ES_InitPorts(r, esResistorPorts);
	r->R = 1.0;
	r->Tc1 = 0.0;
	r->Tc2 = 0.0;
	COMPONENT(r)->dcSimBegin = DC_SimBegin;
	COMPONENT(r)->dcStepBegin = DC_StepBegin;
	COMPONENT(r)->dcStepIter = DC_StepIter;

	M_BindReal(r, "R",	&r->R);
	M_BindReal(r, "Pmax",	&r->Pmax);
	M_BindReal(r, "Tc1",	&r->Tc1);
	M_BindReal(r, "Tc2",	&r->Tc2);
}

static void *
Edit(void *p)
{
	ES_Resistor *r = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealPNZ(box, 0, "ohm", _("Resistance: "), &r->R);
	AG_SeparatorNewHoriz(box);
	M_NumericalNewRealP(box, 0, "W", _("Power rating: "), &r->Pmax);
	M_NumericalNewRealP(box, 0, "mohm/degC", _("Tc1: "), &r->Tc1);
	M_NumericalNewRealP(box, 0, "mohm/degC^2", _("Tc2: "), &r->Tc2);

	return (box);
}

ES_ComponentClass esResistorClass = {
	{
		"Edacious(Circuit:Component:Resistor)"
		"@generic",
		sizeof(ES_Resistor),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		Edit
	},
	N_("Resistor"),
	"R",
	"Generic|Resistances",
	&esIconResistor,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	Export,
	NULL			/* connect */
};
