/*
 * Copyright (c) 2006-2008 Hypertriton, Inc. <http://hypertriton.com/>
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

/*
 * LED component model.
 */

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>

#include "eda.h"
#include "led.h"

const ES_Port esLedPinout[] = {
	{ 0, "",  0.000, 0.625 },
	{ 1, "A", 0.000, 0.000 },
	{ 2, "C", 1.250, 0.000 },
	{ -1 },
};

static void
Draw(void *p, VG *vg)
{
	ES_Led *r = p;

	VG_Begin(vg, VG_POLYGON);
	if (r->state) {
		VG_ColorRGB(vg, 200, 0, 0);
	} else {
		VG_ColorRGB(vg, 0, 0, 0);
	}
	VG_Vertex2(vg, 0.156, -0.240);
	VG_Vertex2(vg, 0.156,  0.240);
	VG_Vertex2(vg, 1.09375,  0.240);
	VG_Vertex2(vg, 1.09375, -0.240);
	VG_End(vg);
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Led *led = p;

	led->Vforw = SC_ReadReal(buf);
	led->Vrev = SC_ReadReal(buf);
	led->I = SC_ReadReal(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Led *led = p;

	SC_WriteReal(buf, led->Vforw);
	SC_WriteReal(buf, led->Vrev);
	SC_WriteReal(buf, led->I);
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

static void
Init(void *p)
{
	ES_Led *r = p;

	ES_ComponentSetPorts(r, esLedPinout);
	r->Vforw = 30e-3;
	r->Vrev = 5.0;
	r->I = 2500e-3;
	r->state = 0;
	COM(r)->intUpdate = ES_LedUpdate;
}

static void *
Edit(void *p)
{
	ES_Led *r = p;
	AG_Window *win;
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

ES_ComponentClass esLedClass = {
	{
		"ES_Component:ES_Led",
		sizeof(ES_Led),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Led"),
	"LED",
	NULL,			/* schem */
	Draw,
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
