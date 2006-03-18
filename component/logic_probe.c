/*	$Csoft$	*/

/*
 * Copyright (c) 2006 Hypertriton, Inc.
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
#include "logic_probe.h"

const AG_Version esLogicProbeVer = {
	"agar-eda logic probe",
	0, 0
};

const ES_ComponentOps esLogicProbeOps = {
	{
		ES_LogicProbeInit,
		NULL,			/* reinit */
		ES_ComponentDestroy,
		ES_LogicProbeLoad,
		ES_LogicProbeSave,
		ES_ComponentEdit
	},
	N_("LogicProbe"),
	"LPROBE",
	ES_LogicProbeDraw,
	ES_LogicProbeEdit,
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};

const ES_Port esLogicProbePinout[] = {
	{ 0, "",  0.000, 0.625 },
	{ 1, "A", 0.000, 0.000 },
	{ -1 },
};

void
ES_LogicProbeDraw(void *p, VG *vg)
{
	ES_LogicProbe *r = p;

	VG_Begin(vg, VG_POLYGON);
	if (r->state) {
		VG_Color3(vg, 200, 0, 0);
	} else {
		VG_Color3(vg, 0, 0, 0);
	}
	VG_Vertex2(vg, 0.156, -0.125);
	VG_Vertex2(vg, 0.156,  0.125);
	VG_Vertex2(vg, 0.500,  0.125);
	VG_Vertex2(vg, 0.500, -0.125);
	VG_End(vg);
}

int
ES_LogicProbeLoad(void *p, AG_Netbuf *buf)
{
	ES_LogicProbe *r = p;

	if (AG_ReadVersion(buf, &esLogicProbeVer, NULL) == -1 ||
	    ES_ComponentLoad(r, buf) == -1)
		return (-1);

	r->Vhigh = SC_ReadReal(buf);
	return (0);
}

int
ES_LogicProbeSave(void *p, AG_Netbuf *buf)
{
	ES_LogicProbe *r = p;

	AG_WriteVersion(buf, &esLogicProbeVer);
	if (ES_ComponentSave(r, buf) == -1)
		return (-1);

	SC_WriteReal(buf, r->Vhigh);
	return (0);
}

void
ES_LogicProbeUpdate(void *p)
{
	ES_LogicProbe *r = p;
	SC_Real v1 = ES_NodeVoltage(COM(r)->ckt,PNODE(r,1));
	SC_Real v2 = ES_NodeVoltage(COM(r)->ckt,PNODE(r,2));

	r->state = ((v1 - v2) >= r->Vhigh);
}

void
ES_LogicProbeInit(void *p, const char *name)
{
	ES_LogicProbe *r = p;

	ES_ComponentInit(r, "logic_probe", name, &esLogicProbeOps,
	    esLogicProbePinout);
	r->Vhigh = 5.0;
	r->state = 0;
	COM(r)->intUpdate = ES_LogicProbeUpdate;
}

#ifdef EDITION
void *
ES_LogicProbeEdit(void *p)
{
	ES_LogicProbe *r = p;
	AG_Window *win;
	AG_Spinbutton *sb;
	AG_FSpinbutton *fsb;

	win = AG_WindowNew(0);

	fsb = AG_FSpinbuttonNew(win, 0, "V", _("HIGH voltage: "));
	AG_WidgetBind(fsb, "value", AG_WIDGET_DOUBLE, &r->Vhigh);
	AG_FSpinbuttonSetMin(fsb, 1.0);
	
	return (win);
}
#endif /* EDITION */
