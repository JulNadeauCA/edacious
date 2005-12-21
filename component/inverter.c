/*	$Csoft: inverter.c,v 1.5 2005/09/27 03:34:09 vedge Exp $	*/

/*
 * Copyright (c) 2004, 2005 CubeSoft Communications, Inc.
 * <http://www.winds-triton.com>
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
#include "inverter.h"

const AG_Version esInverterVer = {
	"agar-eda inverter",
	0, 0
};

const ES_ComponentOps esInverterOps = {
	{
		ES_InverterInit,
		NULL,			/* reinit */
		ES_ComponentDestroy,
		ES_InverterLoad,
		ES_InverterSave,
		ES_ComponentEdit
	},
	N_("Inverter"),
	"Inv",
	ES_InverterDraw,
	ES_InverterEdit,
	NULL,			/* menu */
	NULL,			/* connect */
	NULL,			/* disconnect */
	NULL			/* export */
};

const ES_Port esInverterPinout[] = {
	{ 0, "",	0.0, 1.0 },
	{ 1, "A",	0.0, 0.0 },
	{ 2, "A-bar",	2.0, 0.0 },
	{ 3, "Vcc",	1.0, -1.0 },
	{ 4, "Gnd",	1.0, 1.0 },
	{ -1 },
};

void
ES_InverterDraw(void *p, VG *vg)
{
	ES_Inverter *inv = p;
	ES_Component *com = p;
	VG_Block *block;

	VG_Begin(vg, VG_LINES);
	VG_HLine(vg, 0.00, 0.25, 0.0);
	VG_HLine(vg, 1.75, 2.00, 0.0);
	VG_End(vg);
	VG_Begin(vg, VG_LINE_LOOP);
	VG_Vertex2(vg, 0.25, +0.625);
	VG_Vertex2(vg, 1.75,  0.000);
	VG_Vertex2(vg, 0.25, -0.625);
	VG_End(vg);
	VG_Begin(vg, VG_LINES);
	VG_VintVLine2(vg, 1.000, -1.000, 1.75, 0.0, 0.25, -0.625);
	VG_VintVLine2(vg, 1.000, +1.000, 1.75, 0.0, 0.25, +0.625);
	VG_End(vg);
	VG_Begin(vg, VG_CIRCLE);
	VG_Vertex2(vg, 1.750, 0.0000);
	VG_CircleRadius(vg, 0.0625);
	VG_End(vg);
	VG_Begin(vg, VG_TEXT);
	VG_SetStyle(vg, "component-name");
	VG_Vertex2(vg, 0.750, 0);
	VG_TextAlignment(vg, VG_ALIGN_MC);
	VG_Printf(vg, "%s", AGOBJECT(com)->name);
	VG_End(vg);
}

void
ES_InverterInit(void *p, const char *name)
{
	ES_Inverter *inv = p;

	ES_DigitalInit(inv, "digital.inverter", name, &esInverterOps,
	    esInverterPinout);
	COM(inv)->intStep = ES_InverterStep;
	COM(inv)->intUpdate = ES_InverterUpdate;

	inv->Tphl = 50;
	inv->Tplh = 50;
	inv->Tthl = 80;
	inv->Ttlh = 80;
	inv->Thold = 40;				/* XXX */
	inv->Tehl = 0;
	inv->Telh = 0;
	inv->Cin = 6e-12;
	inv->Cpd = 12e-12;
	inv->Vih = 4;
	inv->Vil = 0;
	inv->Voh = 5;
	inv->Vol = 0;

	digG(inv,4) = 0.900;
	digG(inv,5) = 0.001;
}

/* Increment the counters in response to a simulation step. */
void
ES_InverterStep(void *p, Uint ticks)
{
	ES_Inverter *inv = p;
	
	if (U(inv,1) >= inv->Vih &&
	    U(inv,2) <  inv->Voh) {
		ES_ComponentLog(inv, "H->L hold delay (%u/%u ns)",
		    inv->Tehl, inv->Thold);
		inv->Tehl++;
		inv->Telh = 0;
	} else if (
	    U(inv,1) <= inv->Vil &&
	    U(inv,2) >  inv->Vol) {
		ES_ComponentLog(inv, "L->H hold delay (%.0f/%.0f ns)",
		    inv->Telh*1e9, inv->Thold*1e9);
		inv->Telh++;
		inv->Tehl = 0;
	} else {
		inv->Tehl = 0;
		inv->Telh = 0;
	}
}

/*
 * Update the circuit. This operation might get called multiple times in
 * a single simulation step until stability is reached. It is not allowed
 * to modify the circuit more than once per integration step.
 */
void
ES_InverterUpdate(void *p)
{
	ES_Inverter *inv = p;

	ES_ComponentLog(inv, "u1=%f, u2=%f", U(inv,1), U(inv,2));

	/* Switch OUT when IN != OUT for at least Thold time. */
	if (U(inv,1) >= inv->Vih &&
	    U(inv,2) <  inv->Voh &&
	    inv->Tehl >= inv->Thold) {
		printf("%s: switch OUT to LOW\n", AGOBJECT(inv)->name);
		if (digG(inv,4) > 0.001) { digG(inv,4) -= 0.1; }
		else { inv->Tehl = 0; }
		if (digG(inv,5) < 0.900) { digG(inv,5) += 0.1; }
		else { inv->Tehl = 0; }
	}
	if (U(inv,1) <= inv->Vil &&
	    U(inv,2) >  inv->Vol &&
	    inv->Telh >= inv->Thold) {
		printf("%s: switch OUT to HIGH\n", AGOBJECT(inv)->name);
		if (digG(inv,5) > 0.001) { digG(inv,5) -= 0.1; }
		else { inv->Tehl = 0; }
		if (digG(inv,4) < 0.900) { digG(inv,4) += 0.1; }
		else { inv->Tehl = 0; }
	}
}

int
ES_InverterLoad(void *p, AG_Netbuf *buf)
{
	ES_Inverter *inv = p;

	if (AG_ReadVersion(buf, &esInverterVer, NULL) == -1 ||
	    ES_ComponentLoad(inv, buf) == -1) {
		return (-1);
	}
	inv->Tphl = SC_ReadQTime(buf);
	inv->Tplh = SC_ReadQTime(buf);
	inv->Tthl = SC_ReadQTime(buf);
	inv->Ttlh = SC_ReadQTime(buf);
	inv->Cin = SC_ReadReal(buf);
	inv->Cpd = SC_ReadReal(buf);
	inv->Vih = SC_ReadReal(buf);
	inv->Vil = SC_ReadReal(buf);
	inv->Voh = SC_ReadReal(buf);
	inv->Vol = SC_ReadReal(buf);
	return (0);
}

int
ES_InverterSave(void *p, AG_Netbuf *buf)
{
	ES_Inverter *inv = p;

	AG_WriteVersion(buf, &esInverterVer);
	if (ES_ComponentSave(inv, buf) == -1) {
		return (-1);
	}
	SC_WriteQTime(buf, inv->Tphl);
	SC_WriteQTime(buf, inv->Tplh);
	SC_WriteQTime(buf, inv->Tthl);
	SC_WriteQTime(buf, inv->Ttlh);
	SC_WriteReal(buf, inv->Cin);
	SC_WriteReal(buf, inv->Cpd);
	SC_WriteReal(buf, inv->Vih);
	SC_WriteReal(buf, inv->Vil);
	SC_WriteReal(buf, inv->Voh);
	SC_WriteReal(buf, inv->Vol);
	return (0);
}

#ifdef EDITION
void *
ES_InverterEdit(void *p)
{
	ES_Inverter *inv = p;
	AG_Window *win, *wDig;
	AG_FSpinbutton *fsb;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;
	AG_Box *box;

	win = AG_WindowNew(0);

	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);
	ntab = AG_NotebookAddTab(nb, _("Timings"), AG_BOX_VERT);
	{
		AG_LabelNew(ntab, AG_LABEL_POLLED, "Telh: %uns, Tehl: %uns",
		    &inv->Telh, &inv->Tehl);
		AG_LabelNew(ntab, AG_LABEL_POLLED, "Tplh: %uns, Tphl: %uns",
		    &inv->Tplh, &inv->Tphl);
		AG_LabelNew(ntab, AG_LABEL_POLLED, "Ttlh: %uns, Tthl: %uns",
		    &inv->Ttlh, &inv->Tthl);

		fsb = AG_FSpinbuttonNew(ntab, 0, "ns", "Thold: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_QTIME, &inv->Thold);
		fsb = AG_FSpinbuttonNew(ntab, 0, "ns", "Tphl: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_QTIME, &inv->Tphl);
		fsb = AG_FSpinbuttonNew(ntab, 0, "ns", "Tplh: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_QTIME, &inv->Tplh);
		fsb = AG_FSpinbuttonNew(ntab, 0, "ns", "Tthl: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_QTIME, &inv->Tthl);
		fsb = AG_FSpinbuttonNew(ntab, 0, "ns", "Ttlh: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_QTIME, &inv->Ttlh);
	}

	ntab = AG_NotebookAddTab(nb, _("Voltages"), AG_BOX_VERT);
	{
		fsb = AG_FSpinbuttonNew(ntab, 0, "V", "Vih: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_REAL, &inv->Vih);
		fsb = AG_FSpinbuttonNew(ntab, 0, "V", "Vil: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_REAL, &inv->Vil);
		fsb = AG_FSpinbuttonNew(ntab, 0, "V", "Voh: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_REAL, &inv->Voh);
		fsb = AG_FSpinbuttonNew(ntab, 0, "V", "Vol: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_REAL, &inv->Vol);
	}
	
	ntab = AG_NotebookAddTab(nb, _("Capacitances"), AG_BOX_VERT);
	{
		fsb = AG_FSpinbuttonNew(ntab, 0, "pF", "Cin: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_REAL, &inv->Cin);
		fsb = AG_FSpinbuttonNew(ntab, 0, "pF", "Cpd: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_REAL, &inv->Cpd);
	}
	
	ntab = AG_NotebookAddTab(nb, _("Digital"), AG_BOX_VERT);
	ES_DigitalEdit(inv, ntab);

	return (win);
}
#endif /* EDITION */
