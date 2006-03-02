/*	$Csoft$	*/

/*
 * Copyright (c) 2006 HyperTriton, Inc.
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
#include "led.h"

const AG_Version esLedVer = {
	"agar-eda led",
	0, 0
};

const ES_ComponentOps esLedOps = {
	{
		ES_LedInit,
		NULL,			/* reinit */
		ES_ComponentDestroy,
		ES_LedLoad,
		ES_LedSave,
		ES_ComponentEdit
	},
	N_("Led"),
	"LED",
	ES_LedDraw,
	ES_LedEdit,
	NULL,			/* menu */
	NULL,			/* connect */
	NULL,			/* disconnect */
	NULL			/* export */
};

const ES_Port esLedPinout[] = {
	{ 0, "",  0.000, 0.625 },
	{ 1, "A", 0.000, 0.000 },
	{ 2, "C", 1.250, 0.000 },
	{ -1 },
};

void
ES_LedDraw(void *p, VG *vg)
{
	ES_Led *r = p;

	VG_Begin(vg, VG_POLYGON);
	if (r->state) {
		VG_Color3(vg, 200, 0, 0);
	} else {
		VG_Color3(vg, 0, 0, 0);
	}
	VG_Vertex2(vg, 0.156, -0.240);
	VG_Vertex2(vg, 0.156,  0.240);
	VG_Vertex2(vg, 1.09375,  0.240);
	VG_Vertex2(vg, 1.09375, -0.240);
	VG_End(vg);
}

int
ES_LedLoad(void *p, AG_Netbuf *buf)
{
	ES_Led *r = p;

	if (AG_ReadVersion(buf, &esLedVer, NULL) == -1 ||
	    ES_ComponentLoad(r, buf) == -1)
		return (-1);

	r->Vforw = SC_ReadReal(buf);
	r->Vrev = SC_ReadReal(buf);
	r->I = SC_ReadReal(buf);
	return (0);
}

int
ES_LedSave(void *p, AG_Netbuf *buf)
{
	ES_Led *r = p;

	AG_WriteVersion(buf, &esLedVer);
	if (ES_ComponentSave(r, buf) == -1)
		return (-1);

	SC_WriteReal(buf, r->Vforw);
	SC_WriteReal(buf, r->Vrev);
	SC_WriteReal(buf, r->I);
	return (0);
}

void
ES_LedUpdate(void *p)
{
	ES_Led *r = p;
	SC_Real v1 = ES_NodeVoltage(COM(r)->ckt,PNODE(r,1));
	SC_Real v2 = ES_NodeVoltage(COM(r)->ckt,PNODE(r,2));

	r->state = ((v1 - v2) >= r->Vrev);
}

void
ES_LedInit(void *p, const char *name)
{
	ES_Led *r = p;

	ES_ComponentInit(r, "led", name, &esLedOps, esLedPinout);
	r->Vforw = 30e-3;
	r->Vrev = 5.0;
	r->I = 2500e-3;
	r->state = 0;
	COM(r)->intUpdate = ES_LedUpdate;
}

#ifdef EDITION
void *
ES_LedEdit(void *p)
{
	ES_Led *r = p;
	AG_Window *win;
	AG_Spinbutton *sb;
	AG_FSpinbutton *fsb;

	win = AG_WindowNew(0);

	fsb = AG_FSpinbuttonNew(win, 0, "V", _("Forward voltage: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &r->Vforw);
	AG_FSpinbuttonSetMin(fsb, 1.0);

	fsb = AG_FSpinbuttonNew(win, 0, "V", _("Reverse voltage: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &r->Vrev);
	AG_FSpinbuttonSetMin(fsb, 1.0);
	
	fsb = AG_FSpinbuttonNew(win, 0, "mcd", _("Luminous intensity: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &r->I);
	
	return (win);
}
#endif /* EDITION */
