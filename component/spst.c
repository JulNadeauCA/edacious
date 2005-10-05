/*	$Csoft: spst.c,v 1.5 2005/09/27 03:34:09 vedge Exp $	*/

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

const AG_Version spst_ver = {
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
	NULL,			/* connect */
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
spst_draw(void *p, VG *vg)
{
	struct spst *sw = p;

	VG_Begin(vg, VG_LINES);
	VG_Vertex2(vg, 0.000, 0.000);
	VG_Vertex2(vg, 0.400, 0.000);
	VG_Vertex2(vg, 1.600, 0.000);
	VG_Vertex2(vg, 2.000, 0.000);
	VG_End(vg);
	
	VG_Begin(vg, VG_CIRCLE);
	VG_Vertex2(vg, 0.525, 0.0000);
	VG_CircleRadius(vg, 0.0625);
	VG_End(vg);

	VG_Begin(vg, VG_CIRCLE);
	VG_Vertex2(vg, 1.475, 0.0000);
	VG_CircleRadius(vg, 0.0625);
	VG_End(vg);

	VG_Begin(vg, VG_LINES);
	VG_Vertex2(vg, 0.65, 0.00);
	if (sw->state == 1) {
		VG_Vertex2(vg, 1.35, 0.0);
	} else {
		VG_Vertex2(vg, 1.35, -0.5);
	}
	VG_End(vg);

	VG_Begin(vg, VG_TEXT);
	VG_SetStyle(vg, "component-name");
	VG_Vertex2(vg, 1.000, 0.250);
	VG_TextAlignment(vg, VG_ALIGN_MC);
	VG_Printf(vg, "%s", AGOBJECT(sw)->name);
	VG_End(vg);
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
spst_load(void *p, AG_Netbuf *buf)
{
	struct spst *sw = p;

	if (AG_ReadVersion(buf, &spst_ver, NULL) == -1 ||
	    component_load(sw, buf) == -1)
		return (-1);

	sw->on_resistance = AG_ReadDouble(buf);
	sw->off_resistance = AG_ReadDouble(buf);
	sw->state = (int)AG_ReadUint8(buf);
	return (0);
}

int
spst_save(void *p, AG_Netbuf *buf)
{
	struct spst *sw = p;

	AG_WriteVersion(buf, &spst_ver);
	if (component_save(sw, buf) == -1)
		return (-1);

	AG_WriteDouble(buf, sw->on_resistance);
	AG_WriteDouble(buf, sw->off_resistance);
	AG_WriteUint8(buf, (Uint8)sw->state);
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
		fprintf(f, "R%s %d %d %g\n", AGOBJECT(sw)->name,
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
AG_Window *
spst_edit(void *p)
{
	struct spst *sw = p;
	AG_Window *win, *subwin;
	AG_FSpinbutton *fsb;
	AG_Button *sb;

	win = AG_WindowNew(0);

	fsb = AG_FSpinbuttonNew(win, "ohms", _("ON resistance: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &sw->on_resistance);
	AG_FSpinbuttonSetMin(fsb, 1.0);
	
	fsb = AG_FSpinbuttonNew(win, "ohms", _("OFF resistance: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &sw->off_resistance);
	AG_FSpinbuttonSetMin(fsb, 1.0);

	sb = AG_ButtonNew(win, _("Toggle state"));
	AGWIDGET(sb)->flags |= AG_WIDGET_WFILL|AG_WIDGET_HFILL;
	AG_ButtonSetSticky(sb, 1);
	AG_WidgetBind(sb, "state", AG_WIDGET_BOOL, &sw->state);

	return (win);
}
#endif /* EDITION */
