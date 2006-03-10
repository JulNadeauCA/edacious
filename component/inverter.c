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
		ES_ComponentLoad,
		ES_ComponentSave,
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
	{ 1, "Vcc",	1.0, -1.0 },
	{ 2, "Gnd",	1.0, 1.0 },
	{ 3, "A",	0.0, 0.0 },
	{ 4, "A-bar",	2.0, 0.0 },
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
	COM(inv)->intUpdate = ES_InverterUpdate;
#if 0
	ES_SetSpec(inv, "Tp", _("Propagation delay from A to A-bar"),
	    "Vcc=5;Tamb=25",	0, 50, 90,
	    "Vcc=10;Tamb=25",	0, 30, 60,
	    "Vcc=15;Tamb=25",	0, 25, 50,
	    NULL);
	ES_SetSpec(inv, "Tt", _("Transition time for A-bar"),
	    "Vcc=5;Tamb=25",	0, 80, 150,
	    "Vcc=10;Tamb=25",	0, 50, 100,
	    "Vcc=15;Tamb=25",	0, 40, 80,
	    NULL);
	ES_SetSpec(inv, "Cin", _("Input capacitance"),
	    "Tamb=25", 0, 6, 15,
	    NULL);
	ES_SetSpec(inv, "Cpd", _("Power dissipation capacitance"),
	    "Tamb=25", 0, 6, 15,
	    NULL);
	ES_SetSpec(inv, "Idd", _("Quiescent device current"),
	    "Vcc=5;Tamb=-55",	0, 0, 0.25,
	    "Vcc=10;Tamb=-55",	0, 0, 0.5,
	    "Vcc=15;Tamb=-55",	0, 0, 1.0,
	    "Vcc=5;Tamb=25",	0, 0, 0.25,
	    "Vcc=10;Tamb=25",	0, 0, 0.5,
	    "Vcc=15;Tamb=25",	0, 0, 1.0,
	    "Vcc=5;Tamb=125",	0, 0, 7.5,
	    "Vcc=10;Tamb=125",	0, 0, 15.0,
	    "Vcc=15;Tamb=125",	0, 0, 30.0,
	    NULL);
#endif
	ES_LogicOutput(inv, "A-bar", ES_HI_Z);
}

void
ES_InverterUpdate(void *p)
{
	ES_Inverter *inv = p;

	switch (ES_LogicInput(inv, "A")) {
	case ES_HIGH:
		ES_LogicOutput(inv, "A-bar", ES_LOW);
		break;
	case ES_LOW:
		ES_LogicOutput(inv, "A-bar", ES_HIGH);
		break;
	}
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
#if 0
	ntab = AG_NotebookAddTab(nb, _("Timings"), AG_BOX_VERT);
	{
		AG_LabelNew(ntab, AG_LABEL_POLLED, "Telh: %uns, Tehl: %uns",
		    &inv->Telh, &inv->Tehl);
		AG_LabelNew(ntab, AG_LABEL_POLLED, "Tplh: %uns, Tphl: %uns",
		    &inv->Tplh, &inv->Tphl);

		fsb = AG_FSpinbuttonNew(ntab, 0, "ns", "Thold: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_QTIME, &inv->Thold);
		fsb = AG_FSpinbuttonNew(ntab, 0, "ns", "Tphl: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_QTIME, &inv->Tphl);
		fsb = AG_FSpinbuttonNew(ntab, 0, "ns", "Tplh: ");
		AG_WidgetBind(fsb, "value", SC_WIDGET_QTIME, &inv->Tplh);
	}
#endif
	ntab = AG_NotebookAddTab(nb, _("Digital"), AG_BOX_VERT);
	ES_DigitalEdit(inv, ntab);
	return (win);
}
#endif /* EDITION */
