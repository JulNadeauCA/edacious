/*
 * Copyright (c) 2004-2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Model for an independent (GUI-triggered) SPDT switch.
 * TODO simulate hysteresis, etc.
 */

#include <eda.h>
#include "spdt.h"

const ES_Port esSpdtPorts[] = {
	{ 0, "" },
	{ 1, "A" },
	{ 2, "B" },
	{ 3, "C" },
	{ -1 },
};

#if 0
static void
Draw(void *p, VG_Node *vn)
{
	ES_Spdt *sw = p;
	VG_Point *p1, *p2, *p3, *p4, *p5, *p6;

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
	VG_Vertex2(vg, 1.200, 0.000);
	VG_TextAlignment(vg, VG_ALIGN_MC);
	VG_TextPrintf(vg, "%s", OBJECT(sw)->name);
	VG_End(vg);
}
#endif

static void
Init(void *p)
{
	ES_Spdt *sw = p;

	ES_InitPorts(sw, esSpdtPorts);
	sw->rOn = 1.0;
	sw->rOff = HUGE_VAL;
	sw->state = 1;
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Spdt *sw = p;

	sw->rOn = M_ReadReal(buf);
	sw->rOff = M_ReadReal(buf);
	sw->state = (int)AG_ReadUint8(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Spdt *sw = p;

	M_WriteReal(buf, sw->rOn);
	M_WriteReal(buf, sw->rOff);
	AG_WriteUint8(buf, (Uint8)sw->state);
	return (0);
}

static double
Resistance(void *p, ES_Port *p1, ES_Port *p2)
{
	ES_Spdt *sw = p;

	if (p1->n == 2 && p2->n == 3)
		return (sw->rOff);

	switch (p1->n) {
	case 1:
		switch (p2->n) {
		case 2:
			return (sw->state == 1 ? sw->rOn : sw->rOff);
		case 3:
			return (sw->state == 2 ? sw->rOn : sw->rOff);
		}
		break;
	case 2:
		return (sw->rOff);
	}
	return (-1);
}

static int
Export(void *p, enum circuit_format fmt, FILE *f)
{
	ES_Spdt *sw = p;
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		if (PNODE(sw,1) != -1 &&
		    PNODE(sw,2) != -1) {
			fprintf(f, "R%s %d %d %g\n", OBJECT(sw)->name,
			    PNODE(sw,1), PNODE(sw,2),
			    Resistance(sw, PORT(sw,1), PORT(sw,2)));
		}
		if (PNODE(sw,1) != -1 &&
		    PNODE(sw,3) != -1) {
			fprintf(f, "R%s %d %d %g\n", OBJECT(sw)->name,
			    PNODE(sw,1), PNODE(sw,3),
			    Resistance(sw, PORT(sw,1), PORT(sw,3)));
		}
		if (PNODE(sw,2) != -1 &&
		    PNODE(sw,3) != -1) {
			fprintf(f, "%s %d %d %g\n", OBJECT(sw)->name,
			    PNODE(sw,2), PNODE(sw,3),
			    Resistance(sw, PORT(sw,2), PORT(sw,3)));
		}
		break;
	}
	return (0);
}

static void
ToggleState(AG_Event *event)
{
	ES_Spdt *sw = AG_PTR(1);

	sw->state = (sw->state == 1) ? 2 : 1;
}

static void *
Edit(void *p)
{
	ES_Spdt *sw = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealPNZ(box, 0, "ohm", _("ON resistance: "), &sw->rOn);
	M_NumericalNewRealPNZ(box, 0, "ohm", _("OFF resistance: "), &sw->rOff);
	AG_ButtonAct(box, AG_BUTTON_EXPAND, _("Toggle state"),
	    ToggleState, "%p", sw);
	return (box);
}

ES_ComponentClass esSpdtClass = {
	{
		"ES_Component:ES_Spdt",
		sizeof(ES_Spdt),
		{ 0,0 },
		Init,
		NULL,			/* reinit */
		NULL,			/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Switch (SPDT)"),
	"Sw",
	NULL,			/* schem */
	"Generic|Switches|User Interface",
	&esIconSPDT,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	Export,
	NULL			/* connect */
};
