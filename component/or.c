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
 * Model for the ideal OR gate.
 */

#include <agar/core.h>
#include <agar/vg.h>
#include <agar/gui.h>

#include "eda.h"
#include "or.h"

const ES_Port esOrPinout[] = {
	{ 0, "",	0.0, 1.0 },
	{ 1, "Vcc",	1.0, -1.0 },
	{ 2, "Gnd",	1.0, +1.0 },
	{ 3, "A",	0.0, -0.75 },
	{ 4, "B",	0.0, +0.75 },
	{ 5, "A|B",	2.0, 0.0 },
	{ -1 },
};

static void
Draw(void *p, VG *vg)
{
	ES_Or *gate = p;
	ES_Component *com = p;
	VG_Block *block;

	VG_Begin(vg, VG_LINES);
	VG_HLine(vg, 0.00, 0.25, 0.0);
	VG_HLine(vg, 1.75, 2.00, 0.0);
	VG_End(vg);
	VG_Begin(vg, VG_LINE_LOOP);
	VG_Vertex2(vg, 0.25, +0.625);
	VG_Vertex2(vg, 1.75,  0.000);
	VG_Vertex2(vg, 0.25, -0.625);
	VG_End(vg);
	VG_Begin(vg, VG_LINES);
	VG_VintVLine2(vg, 1.000, -1.000, 1.75, 0.0, 0.25, -0.625);
	VG_VintVLine2(vg, 1.000, +1.000, 1.75, 0.0, 0.25, +0.625);
	VG_End(vg);
	VG_Begin(vg, VG_CIRCLE);
	VG_Vertex2(vg, 1.750, 0.0000);
	VG_CircleRadius(vg, 0.0625);
	VG_End(vg);
	VG_Begin(vg, VG_TEXT);
	VG_SetStyle(vg, "component-name");
	VG_Vertex2(vg, 0.750, 0);
	VG_TextAlignment(vg, VG_ALIGN_MC);
	VG_Printf(vg, "%s", AGOBJECT(com)->name);
	VG_End(vg);
}

static void
Init(void *p)
{
	ES_Or *gate = p;

	COM(gate)->intUpdate = ES_OrUpdate;
	ES_LogicOutput(gate, "A|B", ES_HI_Z);
}

void
ES_OrUpdate(void *p)
{
	ES_Or *gate = p;

	if (ES_LogicInput(gate, "A") == ES_HIGH ||
	    ES_LogicInput(gate, "B") == ES_HIGH) {
		ES_LogicOutput(gate, "A|B", ES_HIGH);
	} else {
		ES_LogicOutput(gate, "A|B", ES_LOW);
	}
}

ES_ComponentClass esOrClass = {
	{
		"ES_Component:ES_Digital:ES_Or",
		sizeof(ES_Or),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		NULL,		/* edit */
	},
	N_("OR Gate"),
	"Or",
	Draw,
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
