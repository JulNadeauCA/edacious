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
 * Base class for "ideal" digital components specified in terms of analog
 * voltage and current behavior.
 */

#include <eda.h>
#include "digital.h"

void
ES_DigitalInitPorts(void *obj, const ES_Port *ports)
{
	ES_Digital *dig = obj;

	ES_InitPorts(dig, ports);
	Debug(dig, "Device has %d pairs\n", COMPONENT(dig)->npairs);
	if ((dig->G = M_NewZero(COMPONENT(dig)->npairs,1)) == NULL)
		AG_FatalError(NULL);
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Digital *dig = p;

	dig->Vcc = M_ReadRange(buf);
	dig->Tamb = M_ReadRange(buf);
	dig->Idd = M_ReadRange(buf);
	dig->Vol = M_ReadRange(buf);
	dig->Voh = M_ReadRange(buf);
	dig->Vil = M_ReadRange(buf);
	dig->Vih = M_ReadRange(buf);
	dig->Iol = M_ReadRange(buf);
	dig->Ioh = M_ReadRange(buf);
	dig->Iin = M_ReadRange(buf);
	dig->Iozh = M_ReadRange(buf);
	dig->Iozl = M_ReadRange(buf);
	dig->Tthl = M_ReadTimeRange(buf);
	dig->Ttlh = M_ReadTimeRange(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Digital *dig = p;

	M_WriteRange(buf, dig->Vcc);
	M_WriteRange(buf, dig->Tamb);
	M_WriteRange(buf, dig->Idd);
	M_WriteRange(buf, dig->Vol);
	M_WriteRange(buf, dig->Voh);
	M_WriteRange(buf, dig->Vil);
	M_WriteRange(buf, dig->Vih);
	M_WriteRange(buf, dig->Iol);
	M_WriteRange(buf, dig->Ioh);
	M_WriteRange(buf, dig->Iin);
	M_WriteRange(buf, dig->Iozh);
	M_WriteRange(buf, dig->Iozl);
	M_WriteTimeRange(buf, dig->Tthl);
	M_WriteTimeRange(buf, dig->Ttlh);
	return (0);
}

void
ES_DigitalStepIter(void *p, ES_SimDC *dc)
{
	ES_Digital *dig = p;
	ES_Node *n;
	Uint i;

	for (i = 0; i < COMPONENT(dig)->npairs; i++) {
		ES_Pair *pair = PAIR(dig,i);
		Uint k = pair->p1->node;
		Uint j = pair->p2->node;
		M_Real g = dig->G->v[i][0];

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
	DIGITAL(p)->VccPort = port;
}

void
ES_DigitalSetGndPort(void *p, int port)
{
	DIGITAL(p)->GndPort = port;
}

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
			dig->G->v[i][0] = (nState == ES_LOW) ?
			    M_TINYVAL : 1.0-M_TINYVAL;
		}
		if (PAIR_MATCHES(pair, port->n, dig->GndPort)) {
			dig->G->v[i][0] = (nState == ES_HIGH) ?
			    M_TINYVAL : 1.0-M_TINYVAL;
		}
	}
	Debug(dig, "LogicOutput: %s -> %s\n", portname,
	    nState == ES_LOW ? "LOW" :
	    nState == ES_HIGH ? "HIGH" : "HI-Z");
	return (0);
}

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
	if (v >= dig->Vih.min) {
		return (ES_HIGH);
	} else if (v <= dig->Vil.max) {
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

	dig->Vcc.min = 3.0;	dig->Vcc.typ = 5.0;	dig->Vcc.max = 15.0;
	dig->Vol.min = 0.0;	dig->Vol.typ = 0.0;	dig->Vol.max = 0.05;
	dig->Voh.min = 4.95;	dig->Voh.typ = 5.0;	dig->Voh.max = HUGE_VAL;
	dig->Vil.min = 0.0;	dig->Vil.typ = 0.0;	dig->Vil.max = 1.0;
	dig->Vih.min = 4.0;	dig->Vih.typ = 5.0;	dig->Vih.max = HUGE_VAL;
	dig->Iol.min = 0.51;	dig->Iol.typ = 0.88;	dig->Iol.max = HUGE_VAL;
	dig->Ioh.min = -0.51;	dig->Ioh.typ = -0.88;	dig->Ioh.max = HUGE_VAL;
	dig->Iin.min = 0.0;	dig->Iin.typ = -10.0e-5; dig->Iin.max = -0.1;
	
	COMPONENT(dig)->dcStepIter = ES_DigitalStepIter;
}

static void
PollPairs(AG_Event *event)
{
	AG_Table *t = AG_SELF();
	ES_Digital *dig = AG_PTR(1);
	int i;

	AG_TableBegin(t);
	for (i = 0; i < COMPONENT(dig)->npairs; i++) {
		ES_Pair *pair = &COMPONENT(dig)->pairs[i];

		AG_TableAddRow(t, "%d:%s:%s:%f", i+1, pair->p1->name,
		    pair->p2->name, dig->G->v[i]);
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
		"ES_Component:ES_Digital",
		sizeof(ES_Digital),
		{ 0,0 },
		Init,
		NULL,			/* reinit */
		NULL,			/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Digital component"),
	"Digital",
	"Digital.eschem",
	"Generic|Digital",
	&esIconANDGate,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
