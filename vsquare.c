/*
 * Copyright (c) 2005-2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Independent square-wave voltage source.
 */

#include <eda.h>
#include "vsquare.h"

const ES_Port esVSquarePorts[] = {
	{  0, "" },
	{  1, "v+" },
	{  2, "v-" },
	{ -1 },
};

static void
Init(void *p)
{
	ES_VSquare *vs = p;

	ES_InitPorts(vs, esVSquarePorts);
	vs->vH = 5.0;
	vs->vL = 0.0;
	vs->t = 0;
	vs->tH = 100;
	vs->tL = 100;
	COMPONENT(vs)->intStep = ES_VSquareStep;
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_VSquare *vs = p;

	vs->vH = M_ReadReal(buf);
	vs->vL = M_ReadReal(buf);
	vs->tH = M_ReadTime(buf);
	vs->tL = M_ReadTime(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_VSquare *vs = p;

	M_WriteReal(buf, vs->vH);
	M_WriteReal(buf, vs->vL);
	M_WriteTime(buf, vs->tH);
	M_WriteTime(buf, vs->tL);
	return (0);
}

static int
Export(void *p, enum circuit_format fmt, FILE *f)
{
	ES_VSquare *vs = p;

	/* TODO */
	return (0);
}

void
ES_VSquareStep(void *p, Uint ticks)
{
	ES_VSquare *vs = p;

	if (VSOURCE(vs)->voltage == vs->vH && ++vs->t > vs->tH) {
		VSOURCE(vs)->voltage = vs->vL;
		vs->t = 0;
	} else if (VSOURCE(vs)->voltage == vs->vL && ++vs->t > vs->tL) {
		VSOURCE(vs)->voltage = vs->vH;
		vs->t = 0;
	} else if (VSOURCE(vs)->voltage != vs->vL &&
	           VSOURCE(vs)->voltage != vs->vH) {
		VSOURCE(vs)->voltage = vs->vL;
	}
}

static void *
Edit(void *p)
{
	ES_VSquare *vs = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewReal(box, 0, "V", _("HIGH voltage: "), &vs->vH);
	M_NumericalNewReal(box, 0, "V", _("LOW voltage: "), &vs->vL);
	M_NumericalNewTime(box, 0, "ns", _("HIGH duration: "), &vs->tH);
	M_NumericalNewTime(box, 0, "ns", _("LOW duration: "), &vs->tL);
	return (box);
}

ES_ComponentClass esVSquareClass = {
	{
		"ES_Component:ES_Vsource:ES_VSquare",
		sizeof(ES_VSquare),
		{ 0,0 },
		Init,
		NULL,		/* free_dataset */
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Voltage source (square)"),
	"Vsq",
	"Sources/Vsquare.eschem",
	"Generic|Sources",
	&esIconVsquare,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	Export,
	NULL			/* connect */
};
