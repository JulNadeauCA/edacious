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
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Resistor *r = p;

	r->Rnom = M_ReadReal(buf);
	r->tolerance = (int)AG_ReadUint32(buf);
	r->Pmax = M_ReadReal(buf);
	r->Tc1 = M_ReadReal(buf);
	r->Tc2 = M_ReadReal(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Resistor *r = p;

	M_WriteReal(buf, r->Rnom);
	AG_WriteUint32(buf, (Uint32)r->tolerance);
	M_WriteReal(buf, r->Pmax);
	M_WriteReal(buf, r->Tc1);
	M_WriteReal(buf, r->Tc2);
	return (0);
}

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
		    PNODE(r,1), PNODE(r,2), r->Rnom);
		break;
	}
	return (0);
}

/*
 * Load the element into the conductance matrix. All conductances between
 * (k,j) are added to (k,k) and (j,j), and subtracted from (k,j) and (j,k).
 *
 *   |  Vk    Vj  | RHS
 *   |------------|-----
 * k |  Gkj  -Gkj |
 * j | -Gkj   Gkj |
 *   |------------|-----
 */
static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_Resistor *r = obj;
	Uint k = PNODE(r,1);
	Uint j = PNODE(r,2);
	M_Real dT = dc->T0 - COMPONENT(r)->Tspec;
	M_Real g;

	if (r->Rnom == 0.0 || k == -1 || j == -1 || (k == 0 && j == 0)) {
		AG_SetError("Null resistance");
		return (-1);
	}
	g = 1.0/(r->Rnom * (1.0 + r->Tc1*dT + r->Tc2*dT*dT));

	InitStampConductance(k, j, r->s, dc);
	
	StampConductance(g, r->s);
	return (0);
}

static void
Init(void *p)
{
	ES_Resistor *r = p;

	ES_InitPorts(r, esResistorPorts);
	r->Rnom = 1;
	r->Pmax = HUGE_VAL;
	r->tolerance = 0;
	r->Tc1 = 0.0;
	r->Tc2 = 0.0;
	COMPONENT(r)->dcSimBegin = DC_SimBegin;
}

static void *
Edit(void *p)
{
	ES_Resistor *r = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealPNZ(box, 0, "ohm", _("Resistance: "), &r->Rnom);
	AG_SeparatorNewHoriz(box);
	AG_NumericalNewIntR(box, 0, "%", _("Tolerance: "), &r->tolerance,
	    0, 100);
	M_NumericalNewRealP(box, 0, "W", _("Power rating: "), &r->Pmax);
	M_NumericalNewRealP(box, 0, "mohm/degC", _("Tc1: "), &r->Tc1);
	M_NumericalNewRealP(box, 0, "mohm/degC^2", _("Tc2: "), &r->Tc2);

	return (box);
}

ES_ComponentClass esResistorClass = {
	{
		"ES_Component:ES_Resistor",
		sizeof(ES_Resistor),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Resistor"),
	"R",
	"Resistor.eschem",
	"Generic|Resistances",
	&esIconResistor,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	Export,
	NULL			/* connect */
};
