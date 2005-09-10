/*	$Csoft: spst.c,v 1.2 2005/09/09 02:50:15 vedge Exp $	*/

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

#include "spst.h"

const struct version spst_ver = {
	"agar-eda spst switch",
	0, 0
};

const struct component_ops spst_ops = {
	{
		spst_init,
		NULL,			/* reinit */
		component_destroy,
		spst_load,
		spst_save,
		component_edit
	},
	N_("SPST switch"),
	"Sw",
	spst_draw,
	spst_edit,
	NULL,			/* configure */
	spst_export,
	NULL,			/* tick */
	spst_resistance,
	NULL,			/* capacitance */
	NULL,			/* inductance */
	NULL			/* isource */
};

const struct pin spst_pinout[] = {
	{ 0, "",  0, 1 },
	{ 1, "A", 0, 0 },
	{ 2, "B", 2, 0 },
	{ -1 },
};

void
spst_draw(void *p, struct vg *vg)
{
	struct spst *sw = p;

	vg_begin_element(vg, VG_LINES);
	vg_vertex2(vg, 0.000, 0.000);
	vg_vertex2(vg, 0.400, 0.000);
	vg_vertex2(vg, 1.600, 0.000);
	vg_vertex2(vg, 2.000, 0.000);
	vg_end_element(vg);
	
	vg_begin_element(vg, VG_CIRCLE);
	vg_vertex2(vg, 0.525, 0.0000);
	vg_circle_radius(vg, 0.0625);
	vg_end_element(vg);

	vg_begin_element(vg, VG_CIRCLE);
	vg_vertex2(vg, 1.475, 0.0000);
	vg_circle_radius(vg, 0.0625);
	vg_end_element(vg);

	vg_begin_element(vg, VG_LINES);
	vg_vertex2(vg, 0.65, 0.00);
	if (sw->state == 1) {
		vg_vertex2(vg, 1.35, 0.0);
	} else {
		vg_vertex2(vg, 1.35, -0.5);
	}
	vg_end_element(vg);

	vg_begin_element(vg, VG_TEXT);
	vg_style(vg, "component-name");
	vg_vertex2(vg, 1.000, 0.250);
	vg_text_align(vg, VG_ALIGN_MC);
	vg_printf(vg, "%s", OBJECT(sw)->name);
	vg_end_element(vg);
}

void
spst_init(void *p, const char *name)
{
	struct spst *sw = p;

	component_init(sw, "spst", name, &spst_ops, spst_pinout);
	sw->on_resistance = 1.0;
	sw->off_resistance = HUGE_VAL;
	sw->state = 0;
}

int
spst_load(void *p, struct netbuf *buf)
{
	struct spst *sw = p;

	if (version_read(buf, &spst_ver, NULL) == -1 ||
	    component_load(sw, buf) == -1)
		return (-1);

	sw->on_resistance = read_double(buf);
	sw->off_resistance = read_double(buf);
	sw->state = (int)read_uint8(buf);
	return (0);
}

int
spst_save(void *p, struct netbuf *buf)
{
	struct spst *sw = p;

	version_write(buf, &spst_ver);
	if (component_save(sw, buf) == -1)
		return (-1);

	write_double(buf, sw->on_resistance);
	write_double(buf, sw->off_resistance);
	write_uint8(buf, (Uint8)sw->state);
	return (0);
}

int
spst_export(void *p, enum circuit_format fmt, FILE *f)
{
	struct spst *sw = p;

	if (PNODE(sw,1) == -1 ||
	    PNODE(sw,2) == -1)
		return (0);
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		fprintf(f, "R%s %d %d %g\n", OBJECT(sw)->name,
		    PNODE(sw,1), PNODE(sw,2),
		    spst_resistance(sw, PIN(sw,1), PIN(sw,2)));
		break;
	}
	return (0);
}

double
spst_resistance(void *p, struct pin *p1, struct pin *p2)
{
	struct spst *sw = p;

	return (sw->state ? sw->on_resistance : sw->off_resistance);
}

#ifdef EDITION
struct window *
spst_edit(void *p)
{
	struct spst *sw = p;
	struct window *win, *subwin;
	struct fspinbutton *fsb;
	struct button *sb;

	win = window_new(0, NULL);

	fsb = fspinbutton_new(win, "ohms", _("ON resistance: "));
	widget_bind(fsb, "value", WIDGET_DOUBLE, &sw->on_resistance);
	fspinbutton_set_min(fsb, 1.0);
	
	fsb = fspinbutton_new(win, "ohms", _("OFF resistance: "));
	widget_bind(fsb, "value", WIDGET_DOUBLE, &sw->off_resistance);
	fspinbutton_set_min(fsb, 1.0);

	sb = button_new(win, _("Toggle state"));
	WIDGET(sb)->flags |= WIDGET_WFILL|WIDGET_HFILL;
	button_set_sticky(sb, 1);
	widget_bind(sb, "state", WIDGET_BOOL, &sw->state);

	return (win);
}
#endif /* EDITION */
