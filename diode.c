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

const ES_Port esDiodePorts[] = {
	{ 0, "" },
	{ 1, "A" },	/* P-side (+) */
	{ 2, "B" },	/* N-side (-) */
	{ -1 },
};

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
        ES_Port *A, *B;

        if ((A = ES_FindPort(d, "A")) == NULL) {
                AG_FatalError(AG_GetError());
                return (-1);
        }

        if ((B = ES_FindPort(d, "B")) == NULL) {
                AG_FatalError(AG_GetError());
                return (-1);
        }

	vA = ES_NodeVoltage(COM(d)->ckt, A->node);
	vB = ES_NodeVoltage(COM(d)->ckt, B->node);
	
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

	Uint k = PNODE(d,1);
	Uint j = PNODE(d,2);

	M_Real I = DiodeLargeSignalModel(d, 0)		/* guess bias current */
	M_Real g = DiodeSmallSignalModel(d, I);	/* small-signal conductance */

	if (k != 0) {
		G->mat[k][k] += g;
	}
	if (j != 0) {
		G->mat[j][j] += g;
	}
	if (k != 0 && j != 0) {
		G->mat[k][j] -= g;
		G->mat[j][k] -= g;
	}
	return (0);
}

static int
LoadDC_RHS(void *p, M_Vector *i, M_Vector *e)
{
        ES_Diode *d = p;

	Uint k = PNODE(d,1);
	Uint j = PNODE(d,2);

	M_Real I   = DiodeLargeSignalModel(d, DiodeVoltageBiasGuess);		/* bias current for voltage guess */
	M_Real Ieq = I-DiodeSmallSignalModel(d, I)*DiodeVoltageBiasGuess; 	/* norton equivalent current source */

	i->v[k] += Ieq;
	i->v[j] -= Ieq;
        return (0);
}

static int
ES_DiodeUpdate(void *p)
{
	ES_Diode *d = p;
	ES_Circuit *ckt = p->ckt;
	
	// TODO 
	// Find a clean way to access G matrix and i vector
	// from simulation
	// G = ??
	// i = ??

	Uint k = PNODE(d,1);
	Uint j = PNODE(d,2);

	M_Real V   = DiodeVoltage(d);
	M_Real I   = DiodeLargeSignalModel(d, V);
	M_Real g   = DiodeSmallSignalModel(d, I);	/* small-signal conductance */
	M_Real Ieq = I-g*V;				/* norton equivalent current source */

	if (k != 0) {
		G->mat[k][k] += g;
	}
	if (j != 0) {
		G->mat[j][j] += g;
	}
	if (k != 0 && j != 0) {
		G->mat[k][j] -= g;
		G->mat[j][k] -= g;
	}

	i->v[k] += Ieq;
	i->v[j] -= Ieq;
        return (0);
}

static void
Init(void *p)
{
	ES_Diode *d = d;

	ES_InitPorts(d, esDiodePorts);
	d->Is = 1e-10;
	d->Vt = 0.025;

	COM(r)->loadDC_G = LoadDC_G;
	COM(r)->loadDC_RHS = LoadDC_RHS;
}

ES_ComponentClass esDiodeClass = {
	{
		"ES_Component:ES_Diode",
		sizeof(ES_Diode),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Diode"),
	"D",
	"Resistor.eschem",	/* temporarily using resistor graphic */
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,
	NULL			/* connect */
};
