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

#include <eda.h>
#include "resistor.h"

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

	r->resistance = M_ReadReal(buf);
	r->tolerance = (int)AG_ReadUint32(buf);
	r->power_rating = M_ReadReal(buf);
	r->Tc1 = M_ReadReal(buf);
	r->Tc2 = M_ReadReal(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Resistor *r = p;

	M_WriteReal(buf, r->resistance);
	AG_WriteUint32(buf, (Uint32)r->tolerance);
	M_WriteReal(buf, r->power_rating);
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
		    PNODE(r,1), PNODE(r,2), r->resistance);
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
static void
ES_ResistorInit(void *p, M_Matrix *G, M_Matrix *B, M_Matrix *C, M_Matrix *D, M_Vector *i, M_Vector *e)
{
	ES_Resistor *r = p;
	ES_Node *n;
	Uint k = PNODE(r,1);
	Uint j = PNODE(r,2);
	M_Real dT = COMCIRCUIT(r)->T0 - COMPONENT(r)->Tspec;
	M_Real g;

	if (r->resistance == 0.0 || k == -1 || j == -1 || (k == 0 && j == 0)) {
		AG_SetError("Null resistance");
	}
	g = 1.0/(r->resistance * (1.0 + r->Tc1*dT + r->Tc2*dT*dT));
	StampConductance(g, k, j, G);
}

static int
LoadSP(void *p, M_Matrix *S, M_Matrix *N)
{
	ES_Resistor *res = p;
	Uint k = PNODE(res,1);
	Uint j = PNODE(res,2);
	M_Real T0 = COMCIRCUIT(res)->T0;
	M_Real Z0 = COMCIRCUIT(res)->Z0;
	M_Real Tspec = COMPONENT(res)->Tspec;
	M_Real r, z, f;
	
	if (res->resistance == 0.0 ||
	    k == -1 || j == -1 || (k == 0 && j == 0)) {
		AG_SetError("null resistance");
		return (-1);
	}

	r = res->resistance;
	z = r/Z0;
	S->v[k][k] += z/(z+2);
	S->v[j][j] += z/(z+2);
	S->v[k][j] -= z/(z+2);
	S->v[j][k] -= z/(z+2);

	f = Tspec*4.0*r*Z0 / ((2.0*Z0 + r)*(2.0*Z0 + r)) / T0;
	N->v[k][k] += f;
	N->v[j][j] += f;
	N->v[k][j] -= f;
	N->v[j][k] -= f;
	return (0);
}

static void
Init(void *p)
{
	ES_Resistor *r = p;

	ES_InitPorts(r, esResistorPorts);
	r->resistance = 1;
	r->power_rating = HUGE_VAL;
	r->tolerance = 0;
	r->Tc1 = 0.0;
	r->Tc2 = 0.0;
	COMPONENT(r)->dcInit = ES_ResistorInit;
	COMPONENT(r)->loadSP = LoadSP;
}

static void *
Edit(void *p)
{
	ES_Resistor *r = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealR(box, 0, "ohm", _("Resistance: "), &r->resistance,
	    M_TINYVAL, HUGE_VAL);
	AG_SeparatorNewHoriz(box);
	AG_NumericalNewIntR(box, 0, "%", _("Tolerance: "), &r->tolerance,
	    0, 100);
	M_NumericalNewRealR(box, 0, "W", _("Power rating: "), &r->power_rating,
	    0.0, HUGE_VAL);
	M_NumericalNewReal(box, 0, "mohm/degC", _("Temp. coeff.: "), &r->Tc1);
	M_NumericalNewReal(box, 0, "mohm/degC^2", _("Temp. coeff.: "), &r->Tc2);

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
