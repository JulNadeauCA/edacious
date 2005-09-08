/*	$Csoft: semiresistor.c,v 1.7 2005/06/17 08:28:54 vedge Exp $	*/

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
#include <engine/widget/spinbutton.h>
#include <engine/widget/fspinbutton.h>
#include <engine/widget/mfspinbutton.h>
#endif

#include "semiresistor.h"

const struct version semiresistor_ver = {
	"agar-eda semiconductor resistor",
	0, 0
};

const struct component_ops semiresistor_ops = {
	{
		semiresistor_init,
		NULL,			/* reinit */
		component_destroy,
		semiresistor_load,
		semiresistor_save,
		component_edit
	},
	N_("Semiconductor resistor"),
	"R",
	semiresistor_draw,
	semiresistor_edit,
	NULL,			/* configure */
	semiresistor_export,
	NULL,			/* tick */
	semiresistor_resistance,
	NULL,			/* capacitance */
	NULL,			/* inductance */
	NULL			/* isource */
};

const struct pin semiresistor_pinout[] = {
	{ 0, "",  0.000, 0.625 },
	{ 1, "A", 0.000, 0.000 },
	{ 2, "B", 1.250, 0.000 },
	{ -1 },
};

void
semiresistor_draw(void *p, struct vg *vg)
{
	struct semiresistor *r = p;

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

	vg_begin_element(vg, VG_TEXT);
	vg_style(vg, "component-name");
	vg_vertex2(vg, 0.625, 0.000);
	vg_text_align(vg, VG_ALIGN_MC);

	vg_printf(vg, "%s\n", OBJECT(r)->name);
	vg_end_element(vg);
}

void
semiresistor_init(void *p, const char *name)
{
	struct semiresistor *r = p;

	component_init(r, "semiresistor", name, &semiresistor_ops,
	    semiresistor_pinout);
	r->l = 2e-6;
	r->w = 1e-6;
	r->rsh = 1000;
	r->defw = 1e-6;
	r->narrow = 0;
	r->Tc1 = 0.0;
	r->Tc2 = 0.0;
}

int
semiresistor_load(void *p, struct netbuf *buf)
{
	struct semiresistor *r = p;

	if (version_read(buf, &semiresistor_ver, NULL) == -1 ||
	    component_load(r, buf) == -1)
		return (-1);

	r->l = read_double(buf);
	r->w = read_double(buf);
	r->rsh = read_double(buf);
	r->defw = read_double(buf);
	r->narrow = read_double(buf);
	r->Tc1 = read_float(buf);
	r->Tc2 = read_float(buf);
	return (0);
}

int
semiresistor_save(void *p, struct netbuf *buf)
{
	struct semiresistor *r = p;

	version_write(buf, &semiresistor_ver);
	if (component_save(r, buf) == -1)
		return (-1);

	write_double(buf, r->l);
	write_double(buf, r->w);
	write_double(buf, r->rsh);
	write_double(buf, r->defw);
	write_double(buf, r->narrow);
	write_float(buf, r->Tc1);
	write_float(buf, r->Tc2);
	return (0);
}

int
semiresistor_export(void *p, enum circuit_format fmt, FILE *f)
{
	struct semiresistor *r = p;
	static int nRmod = 1;

	if (PIN(r,1)->node == NULL ||
	    PIN(r,2)->node == NULL)
		return (0);
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		fprintf(f, ".MODEL Rmod%u R "
		           "(tc1=%g tc2=%g rsh=%g defw=%g narrow=%g "
			   "tnom=%g)\n",
		    nRmod, r->Tc1, r->Tc2, r->rsh, r->defw, r->narrow,
		    COM(r)->Tnom);
		fprintf(f, "%s %u %u Rmod%u L=%g W=%g TEMP=%g\n",
		    OBJECT(r)->name,
		    PIN(r,1)->node->name,
		    PIN(r,2)->node->name,
		    nRmod, r->l, r->w, COM(r)->Tspec);
		nRmod++;
		break;
	}
	return (0);
}

double
semiresistor_resistance(void *p, struct pin *p1, struct pin *p2)
{
	struct semiresistor *r = p;
	double deltaT = COM(r)->Tnom - COM(r)->Tspec;
	double R;

	R = r->rsh*(r->l - r->narrow)/(r->w - r->narrow);
	return (R*(1.0 + r->Tc1*deltaT + r->Tc2*deltaT*deltaT));
}

#ifdef EDITION
struct window *
semiresistor_edit(void *p)
{
	struct semiresistor *r = p;
	struct window *win, *subwin;
	struct spinbutton *sb;
	struct fspinbutton *fsb;
	struct mfspinbutton *mfsb;

	win = window_new(0, NULL);

	mfsb = mfspinbutton_new(win, "um", "x", _("Geometry (LxW): "));
	widget_bind(mfsb, "xvalue", WIDGET_DOUBLE, &r->l);
	widget_bind(mfsb, "yvalue", WIDGET_DOUBLE, &r->w);
	mfspinbutton_set_min(mfsb, 1e-6);
	
	fsb = fspinbutton_new(win, "kohms", _("Sheet resistance/sq: "));
	widget_bind(fsb, "value", WIDGET_DOUBLE, &r->rsh);
	fspinbutton_set_min(fsb, 0);
	
	fsb = fspinbutton_new(win, "um", _("Narrowing due to side etching: "));
	widget_bind(fsb, "value", WIDGET_DOUBLE, &r->narrow);
	fspinbutton_set_min(fsb, 0);
	
	fsb = fspinbutton_new(win, "mohms/degC", _("Temp. coefficient: "));
	widget_bind(fsb, "value", WIDGET_FLOAT, &r->Tc1);
	fsb = fspinbutton_new(win, "mohms/degC^2", _("Temp. coefficient"));
	widget_bind(fsb, "value", WIDGET_FLOAT, &r->Tc2);

	return (win);
}
#endif /* EDITION */
