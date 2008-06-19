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
	{ PORT_P, "P" },	/* P-side (+) */
	{ PORT_N, "N" },	/* N-side (-) */
	{ -1 },
};

/* Returns the voltage across the diode calculated
 * in the last Newton-Raphson iteration */

static M_Real
DiodeVoltage(ES_Diode *d)
{
	return VPORT(d,PORT_P)-VPORT(d,PORT_N);
}

/* Updates the small- and large-signal models, saving the 
 * previous values */

static void
UpdateDiodeModel(ES_Diode *d, M_Real v)
{
	M_Real vDiff = v - d->vPrev;

	if (M_Fabs(vDiff) > d->Vt)
	{
		v = d->vPrev + vDiff/M_Fabs(vDiff)*d->Vt;
	}

	d->gPrev = d->g;
	d->IeqPrev = d->Ieq;
	d->vPrev = v;

	M_Real I =  d->Is*(M_Exp(v/(d->Vt))-1);

	d->g = I/(d->Vt);
	d->Ieq = I-(d->g)*v;
}

static void
ES_DiodeInit(void *p, M_Matrix *G, M_Matrix *B, M_Matrix *C, M_Matrix *D, M_Vector *i, M_Vector *e)
{
        ES_Diode *d = p;

	Uint k = PNODE(d,PORT_P);
	Uint l = PNODE(d,PORT_N);

	M_Real vGuess = 0.7;
	d->vPrev = vGuess;

	UpdateDiodeModel(d, vGuess);

	StampConductance(d->g,k,l,G);
	StampCurrentSource(d->Ieq,l,k,i);
}

static void
ES_DiodeStep(void *p, Uint Telapsed, M_Matrix *G, M_Matrix *B, M_Matrix *C, M_Matrix *D, M_Vector *i, M_Vector *e, const M_Vector *v, const M_Vector *j)
{
        ES_Diode *d = p;

	Uint k = PNODE(d,PORT_P);
	Uint l = PNODE(d,PORT_N);

	M_Real vGuess = 0.7;
	d->vPrev = vGuess;

	UpdateDiodeModel(d, vGuess);

	StampConductance(d->g-d->gPrev,k,l,G);
	StampCurrentSource(d->Ieq-d->IeqPrev,l,k,i);
}

static void
ES_DiodeUpdate(void *p, M_Matrix *G, M_Matrix *B, M_Matrix *C, M_Matrix *D, M_Vector *i, M_Vector *e, const M_Vector *v, const M_Vector *j)
{
	ES_Diode *d = p;

	Uint k = PNODE(d,PORT_P);
	Uint l = PNODE(d,PORT_N);

	UpdateDiodeModel(d, DiodeVoltage(d));

	StampConductance(d->g-d->gPrev,k,l,G);
	StampCurrentSource(d->Ieq-d->IeqPrev,l,k,i); // note that the current source points the opposite direction
}

static void
Init(void *p)
{
	ES_Diode *d = p;

	ES_InitPorts(d, esDiodePorts);
	d->Is = 1e-14;
	d->Vt = 0.025;

	COMPONENT(d)->dcInit = ES_DiodeInit;
	COMPONENT(d)->transientStep = ES_DiodeStep;
	COMPONENT(d)->dcUpdate = ES_DiodeUpdate;
}

static void *
Edit(void *p)
{
	ES_Diode *d = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);
	AG_Numerical *num;

	num = M_NumericalNewRealR(box, 0, "pA", _("Reverse saturation current: "),
	    &d->Is, M_TINYVAL, HUGE_VAL);
//	AG_NumericalSetPrecision(num, "f", 8);

	M_NumericalNewRealR(box, 0, "mV", _("Thermal voltage: "),
	    &d->Vt, M_TINYVAL, HUGE_VAL);

	AG_LabelNewPolledMT(box, AG_LABEL_HFILL, &OBJECT(d)->lock,
	    "Ieq=%f, g=%f", &d->Ieq, &d->g);
	AG_LabelNewPolledMT(box, AG_LABEL_HFILL, &OBJECT(d)->lock,
	    "Prev: v=%f, Ieq=%f, g=%f",
	    &d->vPrev, &d->IeqPrev, &d->gPrev);

	return (box);
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
		Edit
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
