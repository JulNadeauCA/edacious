/*	$Csoft: inverter.c,v 1.8 2005/05/25 07:03:57 vedge Exp $	*/

/*
 * Copyright (c) 2004 Winds Triton Engineering, Inc.
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

const struct version inverter_ver = {
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
	NULL,			/* configure */
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
inverter_draw(void *p, struct vg *vg)
{
	struct inverter *inv = p;
	struct component *com = p;
	struct vg_block *block;

	vg_begin_element(vg, VG_LINES);
	vg_vertex2(vg, 0.000, 0.000);
	vg_vertex2(vg, 0.250, 0.000);
	vg_vertex2(vg, 2.000, 0.000);
	vg_vertex2(vg, 1.750, 0.000);
	vg_end_element(vg);

	vg_begin_element(vg, VG_LINE_LOOP);
	vg_vertex2(vg, 0.250, +0.625);
	vg_vertex2(vg, 1.750,  0.000);
	vg_vertex2(vg, 0.250, -0.625);
	vg_end_element(vg);

	vg_begin_element(vg, VG_CIRCLE);
	vg_vertex2(vg, 1.750, 0.0000);
	vg_circle_radius(vg, 0.0625);
	vg_end_element(vg);

	vg_begin_element(vg, VG_TEXT);
	vg_style(vg, "component-name");
	vg_vertex2(vg, 0.750, 0);
	vg_text_align(vg, VG_ALIGN_MC);
	vg_printf(vg, "%s", OBJECT(com)->name);
	vg_end_element(vg);
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
			event_schedule(NULL, inv, 1, "Abar->1", NULL);
		}
		break;
	case 0:
		if (U(inv,1) <  inv->Vih &&
		    U(inv,2) >= inv->Voh) {
			dv = inv->Voh/inv->Tthl;
			U(inv,2) -= dv;
			event_schedule(NULL, inv, 1, "Abar->0", NULL);
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
	event_new(inv, "Abar->1", switch_pin, "%i, %i", 2, 1);
	event_new(inv, "Abar->0", switch_pin, "%i, %i", 2, 0);
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
			event_schedule(NULL, inv, inv->Tplh, "Abar->1", NULL);
			inv->Telh = 0;
		}
	}

	/* Same for the HIGH-to-LOW transition of A-bar. */
	if (U(com,1) <= inv->Vil &&
	    U(com,2) >  inv->Vol) {
		if (++inv->Tehl >= inv->Thold) {
			event_schedule(NULL, inv, inv->Tphl, "Abar->0", NULL);
			inv->Tehl = 0;
		}
	} else {
		inv->Tehl = 0;
	}
#endif
}

int
inverter_load(void *p, struct netbuf *buf)
{
	struct inverter *inv = p;

	if (version_read(buf, &inverter_ver, NULL) == -1 ||
	    component_load(inv, buf) == -1) {
		return (-1);
	}
	inv->Tphl = read_float(buf);
	inv->Tplh = read_float(buf);
	inv->Tthl = read_float(buf);
	inv->Ttlh = read_float(buf);
	inv->Cin = read_float(buf);
	inv->Cpd = read_float(buf);
	inv->Vih = read_float(buf);
	inv->Vil = read_float(buf);
	inv->Voh = read_float(buf);
	inv->Vol = read_float(buf);
	return (0);
}

int
inverter_save(void *p, struct netbuf *buf)
{
	struct inverter *inv = p;

	version_write(buf, &inverter_ver);
	if (component_save(inv, buf) == -1) {
		return (-1);
	}
	write_float(buf, inv->Tphl);
	write_float(buf, inv->Tplh);
	write_float(buf, inv->Tthl);
	write_float(buf, inv->Ttlh);
	write_float(buf, inv->Cin);
	write_float(buf, inv->Cpd);
	write_float(buf, inv->Vih);
	write_float(buf, inv->Vil);
	write_float(buf, inv->Voh);
	write_float(buf, inv->Vol);
	return (0);
}

int
inverter_export(void *p, enum circuit_format fmt, FILE *f)
{
	struct inverter *inv = p;
	
	if (PIN(inv,1)->node == NULL ||
	    PIN(inv,2)->node == NULL)
		return (0);
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		/* ... */
		break;
	}
	return (0);
}

#ifdef EDITION
struct window *
inverter_edit(void *p)
{
	struct inverter *inv = p;
	struct window *win;
	struct fspinbutton *fsb;

	win = window_new(0, NULL);

	fsb = fspinbutton_new(win, "ns", "Tphl: ");
	widget_bind(fsb, "value", WIDGET_FLOAT, &inv->Tphl);
	fsb = fspinbutton_new(win, "ns", "Tplh: ");
	widget_bind(fsb, "value", WIDGET_FLOAT, &inv->Tplh);
	fsb = fspinbutton_new(win, "ns", "Tthl: ");
	widget_bind(fsb, "value", WIDGET_FLOAT, &inv->Tthl);
	fsb = fspinbutton_new(win, "ns", "Ttlh: ");
	widget_bind(fsb, "value", WIDGET_FLOAT, &inv->Ttlh);
	
	fsb = fspinbutton_new(win, "pF", "Cin: ");
	widget_bind(fsb, "value", WIDGET_FLOAT, &inv->Cin);
	fsb = fspinbutton_new(win, "pF", "Cpd: ");
	widget_bind(fsb, "value", WIDGET_FLOAT, &inv->Cpd);
	
	fsb = fspinbutton_new(win, "V", "Vih: ");
	widget_bind(fsb, "value", WIDGET_FLOAT, &inv->Vih);
	fsb = fspinbutton_new(win, "V", "Vil: ");
	widget_bind(fsb, "value", WIDGET_FLOAT, &inv->Vil);
	fsb = fspinbutton_new(win, "V", "Voh: ");
	widget_bind(fsb, "value", WIDGET_FLOAT, &inv->Voh);
	fsb = fspinbutton_new(win, "V", "Vol: ");
	widget_bind(fsb, "value", WIDGET_FLOAT, &inv->Vol);

	return (win);
}
#endif /* EDITION */
