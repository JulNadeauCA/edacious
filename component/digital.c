/*
 * Copyright (c) 2004-2005 Hypertriton, Inc. <http://hypertriton.com/>
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

#include <agar/core.h>
#include <agar/vg.h>
#include <agar/gui.h>

#include "eda.h"
#include "digital.h"

const ES_ComponentOps esDigitalOps = {
	{
		"ES_Component:ES_Digital",
		sizeof(ES_Digital),
		{ 0,0 },
		NULL,			/* init */
		NULL,			/* reinit */
		NULL,			/* destroy */
		NULL,			/* load */
		NULL,			/* save */
		NULL,			/* edit */
	},
	N_("Digital component"),
	"Digital",
	NULL,			/* draw */
	NULL,			/* edit */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};

void
ES_DigitalDraw(void *p, VG *vg)
{
	ES_Digital *dig = p;

	VG_Begin(vg, VG_LINES);
	VG_HLine(vg, 0.000, 0.15600, 0.000);
	VG_HLine(vg, 1.250, 1.09375, 0.000);
	VG_End(vg);
	VG_Begin(vg, VG_LINE_LOOP);
	VG_Vertex2(vg, 0.156, -0.240);
	VG_Vertex2(vg, 0.156,  0.240);
	VG_Vertex2(vg, 1.09375,  0.240);
	VG_Vertex2(vg, 1.09375, -0.240);
	VG_End(vg);

	VG_Begin(vg, VG_TEXT);
	VG_SetStyle(vg, "component-name");
	VG_Vertex2(vg, 0.625, 0.000);
	VG_TextAlignment(vg, VG_ALIGN_MC);
	VG_Printf(vg, "%s", AGOBJECT(dig)->name);
	VG_End(vg);
}

int
ES_DigitalLoad(void *p, AG_DataSource *buf)
{
	ES_Digital *dig = p;

	if (AG_ReadObjectVersion(buf, dig, NULL) == -1 ||
	    ES_ComponentLoad(dig, buf) == -1)
		return (-1);

	dig->Vcc = SC_ReadRange(buf);
	dig->Tamb = SC_ReadRange(buf);
	dig->Idd = SC_ReadRange(buf);
	dig->Vol = SC_ReadRange(buf);
	dig->Voh = SC_ReadRange(buf);
	dig->Vil = SC_ReadRange(buf);
	dig->Vih = SC_ReadRange(buf);
	dig->Iol = SC_ReadRange(buf);
	dig->Ioh = SC_ReadRange(buf);
	dig->Iin = SC_ReadRange(buf);
	dig->Iozh = SC_ReadRange(buf);
	dig->Iozl = SC_ReadRange(buf);
	dig->Tthl = SC_ReadQTimeRange(buf);
	dig->Ttlh = SC_ReadQTimeRange(buf);
	return (0);
}

int
ES_DigitalSave(void *p, AG_DataSource *buf)
{
	ES_Digital *dig = p;

	AG_WriteObjectVersion(buf, dig);
	if (ES_ComponentSave(dig, buf) == -1)
		return (-1);

	SC_WriteRange(buf, dig->Vcc);
	SC_WriteRange(buf, dig->Tamb);
	SC_WriteRange(buf, dig->Idd);
	SC_WriteRange(buf, dig->Vol);
	SC_WriteRange(buf, dig->Voh);
	SC_WriteRange(buf, dig->Vil);
	SC_WriteRange(buf, dig->Vih);
	SC_WriteRange(buf, dig->Iol);
	SC_WriteRange(buf, dig->Ioh);
	SC_WriteRange(buf, dig->Iin);
	SC_WriteRange(buf, dig->Iozh);
	SC_WriteRange(buf, dig->Iozl);
	SC_WriteQTimeRange(buf, dig->Tthl);
	SC_WriteQTimeRange(buf, dig->Ttlh);
	return (0);
}

/* Stamp the model conductances. */
static int
ES_DigitalLoadDC_G(void *p, SC_Matrix *G)
{
	ES_Digital *dig = p;
	ES_Node *n;
	int i;

	for (i = 0; i < COM(dig)->npairs; i++) {
		ES_Pair *pair = PAIR(dig,i);
		u_int k = pair->p1->node;
		u_int j = pair->p2->node;
		SC_Real g = vEnt(dig->G, i+1);

		if (g == 0.0) {
			continue;
		}
		if (g == HUGE_VAL ||
		    k == -1 || j == -1 || (k == 0 && j == 0)) {
			continue;
		}
		ES_ComponentLog(dig, "pair=(%s,%s) kj=%d,%d, g=%f (idx %d)",
		    pair->p1->name, pair->p2->name, k, j, g, i);

		if (k != 0) {
			mAdd(G, k, k, g);
		}
		if (j != 0) {
			mAdd(G, j, j, g);
		}
		if (k != 0 && j != 0) {
			mSub(G, k, j, g);
			mSub(G, j, k, g);
		}
	}
	return (0);
}

void
ES_DigitalSetVccPort(void *p, int port)
{
	DIG(p)->VccPort = port;
}

void
ES_DigitalSetGndPort(void *p, int port)
{
	DIG(p)->GndPort = port;
}

int
ES_LogicOutput(void *p, const char *portname, ES_LogicState nstate)
{
	ES_Digital *dig = p;
	ES_Port *port;
	int i;

	if ((port = ES_FindPort(dig, portname)) == NULL) {
		AG_FatalError(AG_GetError());
		return (-1);
	}
	for (i = 0; i < COM(dig)->npairs; i++) {
		ES_Pair *pair = &COM(dig)->pairs[i];

		if ((pair->p1 == port && pair->p2->n == dig->VccPort) ||
		    (pair->p2 == port && pair->p1->n == dig->VccPort)) {
			dig->G->mat[i+1][1] = (nstate == ES_LOW) ?
			    1e-9 : 1.0 - 1e-9;
		}
		if ((pair->p1 == port && pair->p2->n == dig->GndPort) ||
		    (pair->p2 == port && pair->p1->n == dig->GndPort)) {
			dig->G->mat[i+1][1] = (nstate == ES_HIGH) ?
			    1e-9 : 1.0 - 1e-9;
		}
	}
	ES_ComponentLog(dig, "%s -> %s", portname,
	    nstate == ES_LOW ? "low" :
	    nstate == ES_HIGH ? "high" : "hi-Z");
	return (0);
}

int
ES_LogicInput(void *p, const char *portname)
{
	ES_Digital *dig = p;
	ES_Port *port;
	SC_Real v;
	
	if ((port = ES_FindPort(dig, portname)) == NULL) {
		AG_FatalError(AG_GetError());
		return (-1);
	}
	v = ES_NodeVoltage(COM(dig)->ckt, port->node);
	printf("[%s:%s]: %f\n", AGOBJECT(dig)->name, portname, v);
	if (v >= dig->Vih.min) {
		return (ES_HIGH);
	} else if (v <= dig->Vil.max) {
		return (ES_LOW);
	} else {
		ES_ComponentLog(dig, "%s: invalid logic level (%fV)",
		    portname, v);
		return (ES_INVALID);
	}
}

void
ES_DigitalInit(void *p, const char *name, const void *ops,
    const ES_Port *pinout)
{
	ES_Digital *dig = p;

	ES_ComponentInit(dig, name, ops, pinout);
	AG_ObjectSetOps(dig, ops);
	dig->VccPort = 1;
	dig->GndPort = 2;
	dig->G = SC_VectorNew(COM(dig)->npairs);
	SC_VectorSetZero(dig->G);

	COM(dig)->loadDC_G = ES_DigitalLoadDC_G;

	dig->Vcc.min = 3.0;	dig->Vcc.typ = 5.0;	dig->Vcc.max = 15.0;
	dig->Vol.min = 0.0;	dig->Vol.typ = 0.0;	dig->Vol.max = 0.05;
	dig->Voh.min = 4.95;	dig->Voh.typ = 5.0;	dig->Voh.max = HUGE_VAL;
	dig->Vil.min = 0.0;	dig->Vil.typ = 0.0;	dig->Vil.max = 1.0;
	dig->Vih.min = 4.0;	dig->Vih.typ = 5.0;	dig->Vih.max = HUGE_VAL;
	dig->Iol.min = 0.51;	dig->Iol.typ = 0.88;	dig->Iol.max = HUGE_VAL;
	dig->Ioh.min = -0.51;	dig->Ioh.typ = -0.88;	dig->Ioh.max = HUGE_VAL;
	dig->Iin.min = 0.0;	dig->Iin.typ = -10.0e-5; dig->Iin.max = -0.1;
}

#ifdef EDITION
static void
PollPairs(AG_Event *event)
{
	AG_Table *t = AG_SELF();
	ES_Digital *dig = AG_PTR(1);
	int i;

	AG_TableBegin(t);
	for (i = 0; i < COM(dig)->npairs; i++) {
		ES_Pair *pair = &COM(dig)->pairs[i];

		AG_TableAddRow(t, "%d:%s:%s:%f", i+1, pair->p1->name,
		    pair->p2->name, vEnt(dig->G, i+1));
	}
	AG_TableEnd(t);
}

void
ES_DigitalEdit(void *p, void *cont)
{
	ES_Digital *dig = p;
	AG_Table *tbl;

	tbl = AG_TableNewPolled(cont, AG_TABLE_EXPAND, PollPairs, "%p", dig);
	AG_TableAddCol(tbl, "n", "<88>", NULL);
	AG_TableAddCol(tbl, "p1", "<88>", NULL);
	AG_TableAddCol(tbl, "p2", "<88>", NULL);
	AG_TableAddCol(tbl, "G", NULL, NULL);
}
#endif /* EDITION */
