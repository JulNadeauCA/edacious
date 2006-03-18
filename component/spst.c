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

#include <agar/core.h>
#include <agar/vg.h>
#include <agar/gui.h>

#include "eda.h"
#include "spst.h"

const AG_Version esSpstVer = {
	"agar-eda spst switch",
	0, 0
};

const ES_ComponentOps esSpstOps = {
	{
		ES_SpstInit,
		NULL,			/* reinit */
		ES_ComponentDestroy,
		ES_SpstLoad,
		ES_SpstSave,
		ES_ComponentEdit
	},
	N_("SPST switch"),
	"Sw",
	ES_SpstDraw,
	ES_SpstEdit,
	ES_SpstInstanceMenu,
	ES_SpstClassMenu,
	ES_SpstExport,
	NULL			/* connect */
};

const ES_Port esSpstPinout[] = {
	{ 0, "",  0, 1 },
	{ 1, "A", 0, 0 },
	{ 2, "B", 2, 0 },
	{ -1 },
};

void
ES_SpstDraw(void *p, VG *vg)
{
	ES_Spst *sw = p;

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
ES_SpstInit(void *p, const char *name)
{
	ES_Spst *sw = p;

	ES_ComponentInit(sw, "spst", name, &esSpstOps, esSpstPinout);
	sw->on_resistance = 1.0;
	sw->off_resistance = HUGE_VAL;
	sw->state = 0;
}

int
ES_SpstLoad(void *p, AG_Netbuf *buf)
{
	ES_Spst *sw = p;

	if (AG_ReadVersion(buf, &esSpstVer, NULL) == -1 ||
	    ES_ComponentLoad(sw, buf) == -1)
		return (-1);

	sw->on_resistance = AG_ReadDouble(buf);
	sw->off_resistance = AG_ReadDouble(buf);
	sw->state = (int)AG_ReadUint8(buf);
	return (0);
}

int
ES_SpstSave(void *p, AG_Netbuf *buf)
{
	ES_Spst *sw = p;

	AG_WriteVersion(buf, &esSpstVer);
	if (ES_ComponentSave(sw, buf) == -1)
		return (-1);

	AG_WriteDouble(buf, sw->on_resistance);
	AG_WriteDouble(buf, sw->off_resistance);
	AG_WriteUint8(buf, (Uint8)sw->state);
	return (0);
}

int
ES_SpstExport(void *p, enum circuit_format fmt, FILE *f)
{
	ES_Spst *sw = p;

	if (PNODE(sw,1) == -1 ||
	    PNODE(sw,2) == -1)
		return (0);
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		fprintf(f, "R%s %d %d %g\n", AGOBJECT(sw)->name,
		    PNODE(sw,1), PNODE(sw,2),
		    ES_SpstResistance(sw, PORT(sw,1), PORT(sw,2)));
		break;
	}
	return (0);
}

double
ES_SpstResistance(void *p, ES_Port *p1, ES_Port *p2)
{
	ES_Spst *sw = p;

	return (sw->state ? sw->on_resistance : sw->off_resistance);
}

static void
SwitchAll(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	int nstate = AG_INT(2);
	ES_Spst *spst;

	AGOBJECT_FOREACH_CLASS(spst, ckt, es_spst, "component.spst") {
		spst->state = (nstate == -1) ? !spst->state : nstate;
	}
}

void
ES_SpstClassMenu(ES_Circuit *ckt, AG_MenuItem *m)
{
	AG_MenuAction(m, _("Switch on all"), EDA_COMPONENT_ICON,
	    SwitchAll, "%p,%i", ckt, 1);
	AG_MenuAction(m, _("Switch off all"), EDA_COMPONENT_ICON,
	    SwitchAll, "%p,%i", ckt, 0);
	AG_MenuAction(m, _("Toggle all"), EDA_COMPONENT_ICON,
	    SwitchAll, "%p,%i", ckt, -1);
}

void
ES_SpstInstanceMenu(void *p, AG_MenuItem *m)
{
	ES_Spst *spst = p;

	AG_MenuIntBool(m, _("Toggle state"), EDA_COMPONENT_ICON,
	    &spst->state, 0);
}

#ifdef EDITION
void *
ES_SpstEdit(void *p)
{
	ES_Spst *sw = p;
	AG_Window *win, *subwin;
	AG_FSpinbutton *fsb;
	AG_Button *sb;

	win = AG_WindowNew(0);

	fsb = AG_FSpinbuttonNew(win, 0, "ohms", _("ON resistance: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &sw->on_resistance);
	AG_FSpinbuttonSetMin(fsb, 1.0);
	
	fsb = AG_FSpinbuttonNew(win, 0, "ohms", _("OFF resistance: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &sw->off_resistance);
	AG_FSpinbuttonSetMin(fsb, 1.0);

	sb = AG_ButtonNew(win, AG_WIDGET_EXPAND, _("Toggle state"));
	AG_ButtonSetSticky(sb, 1);
	AG_WidgetBind(sb, "state", AG_WIDGET_BOOL, &sw->state);

	return (win);
}
#endif /* EDITION */
