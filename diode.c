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
 * Model for the diode.
 */

#include <eda.h>
#include "diode.h"

enum {
	PORT_P = 1,
	PORT_N = 2
};
const ES_Port esDiodePorts[] = {
	{ 0, "" },
	{ 1, "P" },	/* P-side (+) */
	{ 2, "N" },	/* N-side (-) */
	{ -1 },
};

const M_Real DiodeVoltageBiasGuess = 0;

/* Returns the current through the diode for a given voltage bias */

static M_Real
DiodeLargeSignalModel(ES_Diode *d, M_Real vAB)
{
	return d->Is*(M_Exp(vAB/(d->Vt))-1);
}

/* Returns the small-signal conductance of the diode for a given 
 * bias current. */

static M_Real
DiodeSmallSignalModel(ES_Diode *d, M_Real I)
{
	return I/(d->Vt);
}

/* Returns the voltage across the diode calculated
 * in the last Newton-Raphson iteration */

static M_Real
DiodeVoltage(ES_Diode *d)
{
	M_Real vA = VPORT(d,PORT_P);
	M_Real vB = VPORT(d,PORT_N);
	
	return vA-vB;
}

/*
 * Load the element into the conductance matrix. All conductances between
 * (k,j) are added to (k,k) and (j,j), and subtracted from (k,j) and (j,k).
 *
 *   |  Vk    Vj  | RHS
 *   |------------|-----
 * k |  g    -g   |  I
 * j | -g     g   | -I
 *   |------------|-----
 */

static int
LoadDC_G(void *p, M_Matrix *G)
{
	ES_Diode *d = p;

	Uint k = PNODE(d,PORT_P);
	Uint l = PNODE(d,PORT_N);

	M_Real I = DiodeLargeSignalModel(d, DiodeVoltageBiasGuess);	/* guess bias current */
	M_Real g = DiodeSmallSignalModel(d, I);			/* small-signal conductance */

	StampConductance(g,k,l,G);

	return (0);
}

static int
LoadDC_RHS(void *p, M_Vector *i, M_Vector *e)
{
        ES_Diode *d = p;

	Uint k = PNODE(d,PORT_P);
	Uint l = PNODE(d,PORT_N);

	M_Real I   = DiodeLargeSignalModel(d, DiodeVoltageBiasGuess);		/* bias current for voltage guess */
	M_Real Ieq = I-DiodeSmallSignalModel(d, I)*DiodeVoltageBiasGuess; 	/* norton equivalent current source */

	StampCurrentSource(Ieq,k,l,i);

        return (0);
}

static void
ES_DiodeUpdate(void *p, M_Matrix *G, M_Matrix *B, M_Matrix *C, M_Matrix *D, M_Vector *i, M_Vector *e, const M_Vector *v, const M_Vector *j)
{
	ES_Diode *d = p;

	Uint k = PNODE(d,PORT_P);
	Uint l = PNODE(d,PORT_N);

	M_Real V   = DiodeVoltage(d);
	M_Real I   = DiodeLargeSignalModel(d, V);
	M_Real g   = DiodeSmallSignalModel(d, I);	/* small-signal conductance */
	M_Real Ieq = I-g*V;				/* norton equivalent current source */

	StampConductance(g,k,l,G);
	StampCurrentSource(Ieq,k,l,i);
}

static void
Init(void *p)
{
	ES_Diode *d = p;

	ES_InitPorts(d, esDiodePorts);
	d->Is = 1e-10;
	d->Vt = 0.025;

	COM(d)->loadDC_G = LoadDC_G;
	COM(d)->loadDC_RHS = LoadDC_RHS;
	COM(d)->dcUpdate = ES_DiodeUpdate;
}

ES_ComponentClass esDiodeClass = {
	{
		"ES_Component:ES_Diode",
		sizeof(ES_Diode),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		NULL		/* edit */
	},
	N_("Diode"),
	"D",
	"Diode.eschem",
	"Generic|Nonlinear",
	&esIconDiode,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
