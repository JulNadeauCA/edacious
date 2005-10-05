/*	$Csoft: spdt.c,v 1.6 2005/09/29 00:22:35 vedge Exp $	*/

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

#include "spdt.h"

const AG_Version spdt_ver = {
	"agar-eda spdt switch",
	0, 0
};

const struct component_ops spdt_ops = {
	{
		spdt_init,
		NULL,			/* reinit */
		component_destroy,
		spdt_load,
		spdt_save,
		component_edit
	},
	N_("SPDT switch"),
	"Sw",
	spdt_draw,
	spdt_edit,
	NULL,			/* connect */
	spdt_export,
	NULL,			/* tick */
	spdt_resistance,
	NULL,			/* capacitance */
	NULL,			/* inductance */
	NULL			/* isource */
};

const struct pin spdt_pinout[] = {
	{ 0, "",  0, +1.000 },
	{ 1, "A", 0,  0.000 },
	{ 2, "B", 2, -0.500 },
	{ 3, "C", 2, +0.500 },
	{ -1 },
};

void
spdt_draw(void *p, VG *vg)
{
	struct spdt *sw = p;

	VG_Begin(vg, VG_LINES);
	VG_Vertex2(vg, 0.000, 0.000);
	VG_Vertex2(vg, 0.400, 0.000);
	VG_Vertex2(vg, 1.500, -0.500);
	VG_Vertex2(vg, 2.000, -0.500);
	VG_Vertex2(vg, 1.600, +0.500);
	VG_Vertex2(vg, 2.000, +0.500);
	VG_End(vg);
	
	VG_Begin(vg, VG_CIRCLE);
	VG_Vertex2(vg, 0.525, 0.0000);
	VG_CircleRadius(vg, 0.125);
	VG_End(vg);
	VG_Begin(vg, VG_CIRCLE);
	VG_Vertex2(vg, 1.475, -0.500);
	VG_CircleRadius(vg, 0.125);
	VG_End(vg);
	VG_Begin(vg, VG_CIRCLE);
	VG_Vertex2(vg, 1.475, +0.500);
	VG_CircleRadius(vg, 0.125);
	VG_End(vg);

	VG_Begin(vg, VG_LINES);
	VG_Vertex2(vg, 0.65, 0.000);
	switch (sw->state) {
	case 1:
		VG_Vertex2(vg, 1.35, -0.500);
		break;
	case 2:
		VG_Vertex2(vg, 1.35, +0.500);
		break;
	}
	VG_End(vg);

	VG_Begin(vg, VG_TEXT);
	VG_SetStyle(vg, "component-name");
	VG_Vertex2(vg, 1.200, 0.000);
	VG_TextAlignment(vg, VG_ALIGN_MC);
	VG_Printf(vg, "%s", AGOBJECT(sw)->name);
	VG_End(vg);
}

void
spdt_init(void *p, const char *name)
{
	struct spdt *sw = p;

	component_init(sw, "spdt", name, &spdt_ops, spdt_pinout);
	sw->on_resistance = 1.0;
	sw->off_resistance = HUGE_VAL;
	sw->state = 1;
}

int
spdt_load(void *p, AG_Netbuf *buf)
{
	struct spdt *sw = p;

	if (AG_ReadVersion(buf, &spdt_ver, NULL) == -1 ||
	    component_load(sw, buf) == -1)
		return (-1);

	sw->on_resistance = AG_ReadDouble(buf);
	sw->off_resistance = AG_ReadDouble(buf);
	sw->state = (int)AG_ReadUint8(buf);
	return (0);
}

int
spdt_save(void *p, AG_Netbuf *buf)
{
	struct spdt *sw = p;

	AG_WriteVersion(buf, &spdt_ver);
	if (component_save(sw, buf) == -1)
		return (-1);

	AG_WriteDouble(buf, sw->on_resistance);
	AG_WriteDouble(buf, sw->off_resistance);
	AG_WriteUint8(buf, (Uint8)sw->state);
	return (0);
}

int
spdt_export(void *p, enum circuit_format fmt, FILE *f)
{
	struct spdt *sw = p;
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		if (PNODE(sw,1) != -1 &&
		    PNODE(sw,2) != -1) {
			fprintf(f, "R%s %d %d %g\n", AGOBJECT(sw)->name,
			    PNODE(sw,1), PNODE(sw,2),
			    spdt_resistance(sw, PIN(sw,1), PIN(sw,2)));
		}
		if (PNODE(sw,1) != -1 &&
		    PNODE(sw,3) != -1) {
			fprintf(f, "R%s %d %d %g\n", AGOBJECT(sw)->name,
			    PNODE(sw,1), PNODE(sw,3),
			    spdt_resistance(sw, PIN(sw,1), PIN(sw,3)));
		}
		if (PNODE(sw,2) != -1 &&
		    PNODE(sw,3) != -1) {
			fprintf(f, "%s %d %d %g\n", AGOBJECT(sw)->name,
			    PNODE(sw,2), PNODE(sw,3),
			    spdt_resistance(sw, PIN(sw,2), PIN(sw,3)));
		}
		break;
	}
	return (0);
}

double
spdt_resistance(void *p, struct pin *p1, struct pin *p2)
{
	struct spdt *sw = p;

	if (p1->n == 2 && p2->n == 3)
		return (sw->off_resistance);

	switch (p1->n) {
	case 1:
		switch (p2->n) {
		case 2:
			return (sw->state == 1 ? sw->on_resistance :
			                         sw->off_resistance);
		case 3:
			return (sw->state == 2 ? sw->on_resistance :
			                         sw->off_resistance);
		}
		break;
	case 2:
		return (sw->off_resistance);
	}
	return (-1);
}

#ifdef EDITION
static void
toggle_state(int argc, union evarg *argv)
{
	struct spdt *sw = argv[1].p;

	sw->state = (sw->state == 1) ? 2 : 1;
}

AG_Window *
spdt_edit(void *p)
{
	struct spdt *sw = p;
	AG_Window *win;
	AG_FSpinbutton *fsb;

	win = AG_WindowNew(0);

	fsb = AG_FSpinbuttonNew(win, "ohms", _("ON resistance: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &sw->on_resistance);
	AG_FSpinbuttonSetMin(fsb, 1.0);
	
	fsb = AG_FSpinbuttonNew(win, "ohms", _("OFF resistance: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &sw->off_resistance);
	AG_FSpinbuttonSetMin(fsb, 1.0);

	AG_ButtonAct(win, _("Toggle state"), AG_BUTTON_WFILL|AG_BUTTON_HFILL,
	    toggle_state, "%p", sw);

	return (win);
}
#endif /* EDITION */
