/*	$Csoft: resistor.c,v 1.2 2005/09/09 02:50:15 vedge Exp $	*/

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
#include <engine/widget/spinbutton.h>
#include <engine/widget/fspinbutton.h>
#endif

#include "resistor.h"

const struct version resistor_ver = {
	"agar-eda resistor",
	0, 0
};

const struct component_ops resistor_ops = {
	{
		resistor_init,
		NULL,			/* reinit */
		component_destroy,
		resistor_load,
		resistor_save,
		component_edit
	},
	N_("Resistor"),
	"R",
	resistor_draw,
	resistor_edit,
	resistor_configure,
	resistor_export,
	NULL,			/* tick */
	resistor_resistance,
	NULL,			/* capacitance */
	NULL,			/* inductance */
	NULL			/* isource */
};

const struct pin resistor_pinout[] = {
	{ 0, "",  0.000, 0.625 },
	{ 1, "A", 0.000, 0.000 },
	{ 2, "B", 1.250, 0.000 },
	{ -1 },
};

enum {
	RESISTOR_BOX_STYLE,
	RESISTOR_LINE_STYLE
} resistor_style = 0;

int resistor_dispval = 0;

void
resistor_draw(void *p, struct vg *vg)
{
	struct resistor *r = p;

	switch (resistor_style) {
	case RESISTOR_BOX_STYLE:
		vg_begin_element(vg, VG_LINES);
		vg_vertex2(vg, 0.000, 0.000);
		vg_vertex2(vg, 0.156, 0.000);
		vg_vertex2(vg, 1.250, 0.000);
		vg_vertex2(vg, 1.09375, 0.000);
		vg_end_element(vg);
		vg_begin_element(vg, VG_LINE_LOOP);
		vg_vertex2(vg, 0.156, -0.240);
		vg_vertex2(vg, 0.156,  0.240);
		vg_vertex2(vg, 1.09375,  0.240);
		vg_vertex2(vg, 1.09375, -0.240);
		vg_end_element(vg);
		break;
	case RESISTOR_LINE_STYLE:
		vg_begin_element(vg, VG_LINE_STRIP);
		vg_vertex2(vg, 0.000, 0.125);
		vg_vertex2(vg, 0.125, 0.000);
		vg_vertex2(vg, 0.250, 0.125);
		vg_vertex2(vg, 0.375, 0.000);
		vg_vertex2(vg, 0.500, 0.125);
		vg_vertex2(vg, 0.625, 0.000);
		vg_vertex2(vg, 0.750, 0.125);
		vg_vertex2(vg, 0.875, 0.000);
		vg_vertex2(vg, 1.000, 0.000);
		vg_end_element(vg);
		break;
	}

	vg_begin_element(vg, VG_TEXT);
	vg_style(vg, "component-name");
	vg_vertex2(vg, 0.625, 0.000);
	vg_text_align(vg, VG_ALIGN_MC);

	if (resistor_dispval) {
		vg_printf(vg, "%s\n%.2f \xce\xa9\n", OBJECT(r)->name,
		    r->resistance);
	} else {
		vg_printf(vg, "%s\n", OBJECT(r)->name);
	}
	vg_end_element(vg);
}

int
resistor_configure(void *p)
{
	struct resistor *r = p;
#if 0
	text_prompt_float(&r->resistance, 0, HUGE_VAL, "ohms",
	    _("Enter %s's resistance: "), OBJECT(r)->name);
#endif
	return (0);
}

void
resistor_init(void *p, const char *name)
{
	struct resistor *r = p;

	component_init(r, "resistor", name, &resistor_ops, resistor_pinout);
	r->resistance = 1;
	r->power_rating = HUGE_VAL;
	r->tolerance = 0;
}

int
resistor_load(void *p, struct netbuf *buf)
{
	struct resistor *r = p;

	if (version_read(buf, &resistor_ver, NULL) == -1 ||
	    component_load(r, buf) == -1)
		return (-1);

	r->resistance = read_double(buf);
	r->tolerance = (int)read_uint32(buf);
	r->power_rating = read_double(buf);
	r->Tc1 = read_float(buf);
	r->Tc2 = read_float(buf);
	return (0);
}

int
resistor_save(void *p, struct netbuf *buf)
{
	struct resistor *r = p;

	version_write(buf, &resistor_ver);
	if (component_save(r, buf) == -1)
		return (-1);

	write_double(buf, r->resistance);
	write_uint32(buf, (Uint32)r->tolerance);
	write_double(buf, r->power_rating);
	write_float(buf, r->Tc1);
	write_float(buf, r->Tc2);
	return (0);
}

int
resistor_export(void *p, enum circuit_format fmt, FILE *f)
{
	struct resistor *r = p;

	if (PNODE(r,1) == -1 ||
	    PNODE(r,2) == -1)
		return (0);
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		fprintf(f, "%s %d %d %g\n", OBJECT(r)->name,
		    PNODE(r,1), PNODE(r,2), r->resistance);
		break;
	}
	return (0);
}

double
resistor_resistance(void *p, struct pin *p1, struct pin *p2)
{
	struct resistor *r = p;
	double deltaT = COM(r)->Tnom - COM(r)->Tspec;

	return (r->resistance * (1.0 + r->Tc1*deltaT + r->Tc2*deltaT*deltaT));
}

#ifdef EDITION
struct window *
resistor_edit(void *p)
{
	struct resistor *r = p;
	struct window *win;
	struct spinbutton *sb;
	struct fspinbutton *fsb;

	win = window_new(0, NULL);

	fsb = fspinbutton_new(win, "ohms", _("Resistance: "));
	widget_bind(fsb, "value", WIDGET_DOUBLE, &r->resistance);
	fspinbutton_set_min(fsb, 1.0);
	
	sb = spinbutton_new(win, _("Tolerance (%%): "));
	widget_bind(sb, "value", WIDGET_INT, &r->tolerance);
	spinbutton_set_range(sb, 1, 100);

	fsb = fspinbutton_new(win, "W", _("Power rating: "));
	widget_bind(fsb, "value", WIDGET_DOUBLE, &r->power_rating);
	fspinbutton_set_min(fsb, 0);
	
	fsb = fspinbutton_new(win, "mohms/degC", _("Temp. coefficient: "));
	widget_bind(fsb, "value", WIDGET_FLOAT, &r->Tc1);
	fsb = fspinbutton_new(win, "mohms/degC^2", _("Temp. coefficient: "));
	widget_bind(fsb, "value", WIDGET_FLOAT, &r->Tc2);

	return (win);
}
#endif /* EDITION */
