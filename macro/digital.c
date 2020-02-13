/*
 * Copyright (c) 2004-2020 Julien Nadeau Carriere (vedge@csoft.net)
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
 * Base class for "ideal" digital components specified in terms of analog
 * voltage and current behavior.
 */

#include <core/core.h>
#include "macro.h"

void
ES_DigitalInitPorts(void *obj, const ES_Port *ports)
{
	ES_Digital *dig = obj;

	ES_InitPorts(dig, ports);
	Debug(dig, "Device has %d pairs\n", COMPONENT(dig)->npairs);
	if ((dig->G = M_New(COMPONENT(dig)->npairs,1)) == NULL) {
		AG_FatalError(NULL);
	}
	M_SetZero(dig->G);
}

void
ES_DigitalStepIter(void *p, ES_SimDC *dc)
{
	ES_Digital *dig = p;
	Uint i;

	for (i = 0; i < COMPONENT(dig)->npairs; i++) {
		ES_Pair *pair = PAIR(dig,i);
		Uint k = pair->p1->node;
		Uint j = pair->p2->node;
		M_Real g = M_VecGet(dig->G, i);

		if (g == 0.0) {
			continue;
		}
		if (g == HUGE_VAL ||
		    k == -1 || j == -1 || (k == 0 && j == 0)) {
			continue;
		}
		/* TODO : port to new stamp system */
		/* StampConductance(g, k,j, dc); */
	}
}

void
ES_DigitalSetVccPort(void *p, int port)
{
	ESDIGITAL(p)->VccPort = port;
}

void
ES_DigitalSetGndPort(void *p, int port)
{
	ESDIGITAL(p)->GndPort = port;
}

#define PAIR_MATCHES(pair,a,b) \
	((pair->p1->n == (a) && pair->p2->n == (b)) || \
	 (pair->p2->n == (a) && pair->p1->n == (b)))

int
ES_LogicOutput(void *p, const char *portname, ES_LogicState nState)
{
	ES_Digital *dig = p;
	ES_Port *port;
	int i;

	if ((port = ES_FindPort(dig, portname)) == NULL) {
		AG_FatalError(AG_GetError());
		return (-1);
	}
	for (i = 0; i < COMPONENT(dig)->npairs; i++) {
		ES_Pair *pair = &COMPONENT(dig)->pairs[i];

		if (PAIR_MATCHES(pair, port->n, dig->VccPort)) {
			M_VecSet(dig->G, i,
				 (nState == ES_LOW) ?
				 M_TINYVAL : 1.0-M_TINYVAL);
		}
		if (PAIR_MATCHES(pair, port->n, dig->GndPort)) {
			M_VecSet(dig->G, i,
				 (nState == ES_HIGH) ?
				 M_TINYVAL : 1.0-M_TINYVAL);
		}
	}
	Debug(dig, "LogicOutput: %s -> %s\n", portname,
	    nState == ES_LOW ? "LOW" :
	    nState == ES_HIGH ? "HIGH" : "HI-Z");
	return (0);
}

#undef PAIR_MATCHES

int
ES_LogicInput(void *p, const char *portname)
{
	ES_Digital *dig = p;
	ES_Port *port;
	M_Real v;
	
	if ((port = ES_FindPort(dig, portname)) == NULL) {
		AG_FatalError(AG_GetError());
		return (-1);
	}
	v = ES_NodeVoltage(COMPONENT(dig)->ckt, port->node);
	if (v >= dig->ViH_min) {
		return (ES_HIGH);
	} else if (v <= dig->ViL_max) {
		return (ES_LOW);
	} else {
		Debug(dig, "%s: Invalid logic level (%fV)", portname, v);
		return (ES_INVALID);
	}
}

static void
Init(void *obj)
{
	ES_Digital *dig = obj;

	dig->VccPort = 1;
	dig->GndPort = 2;
	dig->G = NULL;

	dig->Vcc = 5.0;
	dig->VoL = 0.0;
	dig->VoH = 5.0;
	dig->ViH_min = 4.0;
	dig->ViL_max = 1.0;
	dig->Iin = -10.0e-5;
	dig->Idd = 0.0;
	dig->IoL = 0.88;	dig->IoH = -0.88;
	dig->tHL = 10;		dig->tLH = 10;
	dig->IozH = 0.0;	dig->IozL = 0.0;

	COMPONENT(dig)->dcStepIter = ES_DigitalStepIter;
	
	M_BindReal(dig, "Vcc",	&dig->Vcc);
	M_BindReal(dig, "VoL",	&dig->VoL);
	M_BindReal(dig, "VoH",	&dig->VoH);
	M_BindReal(dig, "ViH_min", &dig->ViH_min);
	M_BindReal(dig, "ViL_max", &dig->ViL_max);
	M_BindReal(dig, "Idd",	&dig->Idd);
	M_BindReal(dig, "IoL",	&dig->IoL);
	M_BindReal(dig, "IoH",	&dig->IoH);
	M_BindReal(dig, "Iin",	&dig->Iin);
	M_BindReal(dig, "IozH",	&dig->IozH);
	M_BindReal(dig, "IozL",	&dig->IozL);
	M_BindTime(dig, "tHL",	&dig->tHL);
	M_BindTime(dig, "tLH",	&dig->tLH);
}

static void
PollPairs(AG_Event *event)
{
	AG_Table *t = AG_TABLE_SELF();
	ES_Digital *dig = ES_DIGITAL_PTR(1);
	int i;

	AG_TableBegin(t);
	for (i = 0; i < COMPONENT(dig)->npairs; i++) {
		ES_Pair *pair = &COMPONENT(dig)->pairs[i];

		AG_TableAddRow(t, "%d:%s:%s:%f", i+1, pair->p1->name,
			       pair->p2->name, M_VecGet(dig->G, i));
	}
	AG_TableEnd(t);
}

static void *
Edit(void *p)
{
	ES_Digital *dig = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);
	AG_Table *tbl;
	
	AG_LabelNew(box, 0, _("Pairs:"));
	tbl = AG_TableNewPolled(box, AG_TABLE_EXPAND, PollPairs, "%p", dig);
	AG_TableAddCol(tbl, "n", "<88>", NULL);
	AG_TableAddCol(tbl, "p1", "<88>", NULL);
	AG_TableAddCol(tbl, "p2", "<88>", NULL);
	AG_TableAddCol(tbl, "G", NULL, NULL);

	return (box);
}

ES_ComponentClass esDigitalClass = {
	{
		"Edacious(Circuit:Component:Digital)"
		"@macro",
		sizeof(ES_Digital),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		Edit
	},
	N_("Digital component"),
	"Digital",
	"Generic|Digital",
	&esIconANDGate,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
