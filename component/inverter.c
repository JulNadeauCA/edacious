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

#include <engine/engine.h>
#include <engine/vg/vg.h>

#ifdef EDITION
#include <engine/widget/window.h>
#include <engine/widget/fspinbutton.h>
#endif

#include "inverter.h"

const AG_Version inverter_ver = {
	"agar-eda inverter",
	0, 0
};

const struct component_ops inverter_ops = {
	{
		inverter_init,
		NULL,			/* reinit */
		component_destroy,
		inverter_load,
		inverter_save,
		component_edit
	},
	N_("Inverter"),
	"Inv",
	inverter_draw,
	inverter_edit,
	NULL,			/* connect */
	inverter_export,
	inverter_tick,
	NULL,			/* resistance */
	NULL,			/* capacitance */
	NULL,			/* inductance */
	NULL			/* isource */
};

const struct pin inverter_pinout[] = {
	{ 0, "",	0.0, 1.0 },
	{ 1, "A",	0.0, 0.0 },
	{ 2, "A-bar",	2.0, 0.0 },
	{ -1 },
};

void
inverter_draw(void *p, VG *vg)
{
	struct inverter *inv = p;
	struct component *com = p;
	VG_Block *block;

	VG_Begin(vg, VG_LINES);
	VG_Vertex2(vg, 0.000, 0.000);
	VG_Vertex2(vg, 0.250, 0.000);
	VG_Vertex2(vg, 2.000, 0.000);
	VG_Vertex2(vg, 1.750, 0.000);
	VG_End(vg);

	VG_Begin(vg, VG_LINE_LOOP);
	VG_Vertex2(vg, 0.250, +0.625);
	VG_Vertex2(vg, 1.750,  0.000);
	VG_Vertex2(vg, 0.250, -0.625);
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

/* Initiate a LTH/HTL transition on a given pin. */
static void
switch_pin(int argc, union evarg *argv)
{
	struct inverter *inv = argv[0].p;
	int pin = argv[1].i;
	int level = argv[2].i;
	float dv;

	dprintf("pin = %d, level = %d\n", pin, level);

	switch (level) {
	case 1:
		if (U(inv,1) >= inv->Vih &&
		    U(inv,2) <  inv->Voh) {
			dv = inv->Voh/inv->Ttlh;
			U(inv,2) += dv;
			AG_SchedEvent(NULL, inv, 1, "Abar->1", NULL);
		}
		break;
	case 0:
		if (U(inv,1) <  inv->Vih &&
		    U(inv,2) >= inv->Voh) {
			dv = inv->Voh/inv->Tthl;
			U(inv,2) -= dv;
			AG_SchedEvent(NULL, inv, 1, "Abar->0", NULL);
		}
		break;
	}
}

void
inverter_init(void *p, const char *name)
{
	struct inverter *inv = p;

	/* Default parameters: CD4069UBC (Fairchild 04/2002). */
	component_init(inv, "inverter", name, &inverter_ops, inverter_pinout);
	inv->Tphl = 50;
	inv->Tplh = 50;
	inv->Tthl = 80;
	inv->Ttlh = 80;
	inv->Thold = 40;				/* XXX */
	inv->Tehl = 0;
	inv->Telh = 0;
	inv->Cin = 6;
	inv->Cpd = 12;
	inv->Vih = 4;
	inv->Vil = 0;
	inv->Voh = 5;
	inv->Vol = 0;
	AG_SetEvent(inv, "Abar->1", switch_pin, "%i, %i", 2, 1);
	AG_SetEvent(inv, "Abar->0", switch_pin, "%i, %i", 2, 0);
}

void
inverter_tick(void *p)
{
	struct inverter *inv = p;
	struct component *com = p;

#if 0
	/*
	 * Initiate the transition of A-bar in Tphl/Tplh ns if the voltage 
	 * at A is maintained at >=Vih during at least Thold ns.
	 */
	if (U(com,1) >= inv->Vih &&
	    U(com,2) <  inv->Voh) {
		if (++inv->Telh >= inv->Thold) {
			AG_SchedEvent(NULL, inv, inv->Tplh, "Abar->1", NULL);
			inv->Telh = 0;
		}
	}

	/* Same for the HIGH-to-LOW transition of A-bar. */
	if (U(com,1) <= inv->Vil &&
	    U(com,2) >  inv->Vol) {
		if (++inv->Tehl >= inv->Thold) {
			AG_SchedEvent(NULL, inv, inv->Tphl, "Abar->0", NULL);
			inv->Tehl = 0;
		}
	} else {
		inv->Tehl = 0;
	}
#endif
}

int
inverter_load(void *p, AG_Netbuf *buf)
{
	struct inverter *inv = p;

	if (AG_ReadVersion(buf, &inverter_ver, NULL) == -1 ||
	    component_load(inv, buf) == -1) {
		return (-1);
	}
	inv->Tphl = AG_ReadFloat(buf);
	inv->Tplh = AG_ReadFloat(buf);
	inv->Tthl = AG_ReadFloat(buf);
	inv->Ttlh = AG_ReadFloat(buf);
	inv->Cin = AG_ReadFloat(buf);
	inv->Cpd = AG_ReadFloat(buf);
	inv->Vih = AG_ReadFloat(buf);
	inv->Vil = AG_ReadFloat(buf);
	inv->Voh = AG_ReadFloat(buf);
	inv->Vol = AG_ReadFloat(buf);
	return (0);
}

int
inverter_save(void *p, AG_Netbuf *buf)
{
	struct inverter *inv = p;

	AG_WriteVersion(buf, &inverter_ver);
	if (component_save(inv, buf) == -1) {
		return (-1);
	}
	AG_WriteFloat(buf, inv->Tphl);
	AG_WriteFloat(buf, inv->Tplh);
	AG_WriteFloat(buf, inv->Tthl);
	AG_WriteFloat(buf, inv->Ttlh);
	AG_WriteFloat(buf, inv->Cin);
	AG_WriteFloat(buf, inv->Cpd);
	AG_WriteFloat(buf, inv->Vih);
	AG_WriteFloat(buf, inv->Vil);
	AG_WriteFloat(buf, inv->Voh);
	AG_WriteFloat(buf, inv->Vol);
	return (0);
}

int
inverter_export(void *p, enum circuit_format fmt, FILE *f)
{
	struct inverter *inv = p;
	
	if (PNODE(inv,1) == -1 ||
	    PNODE(inv,2) == -1)
		return (0);
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		/* ... */
		break;
	}
	return (0);
}

#ifdef EDITION
AG_Window *
inverter_edit(void *p)
{
	struct inverter *inv = p;
	AG_Window *win;
	AG_FSpinbutton *fsb;

	win = AG_WindowNew(0);

	fsb = AG_FSpinbuttonNew(win, "ns", "Tphl: ");
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &inv->Tphl);
	fsb = AG_FSpinbuttonNew(win, "ns", "Tplh: ");
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &inv->Tplh);
	fsb = AG_FSpinbuttonNew(win, "ns", "Tthl: ");
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &inv->Tthl);
	fsb = AG_FSpinbuttonNew(win, "ns", "Ttlh: ");
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &inv->Ttlh);
	
	fsb = AG_FSpinbuttonNew(win, "pF", "Cin: ");
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &inv->Cin);
	fsb = AG_FSpinbuttonNew(win, "pF", "Cpd: ");
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &inv->Cpd);
	
	fsb = AG_FSpinbuttonNew(win, "V", "Vih: ");
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &inv->Vih);
	fsb = AG_FSpinbuttonNew(win, "V", "Vil: ");
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &inv->Vil);
	fsb = AG_FSpinbuttonNew(win, "V", "Voh: ");
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &inv->Voh);
	fsb = AG_FSpinbuttonNew(win, "V", "Vol: ");
	AG_WidgetBind(fsb, "value", AG_WIDGET_FLOAT, &inv->Vol);

	return (win);
}
#endif /* EDITION */
