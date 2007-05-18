/*	$Csoft: inverter.c,v 1.5 2005/09/27 03:34:09 vedge Exp $	*/

/*
 * Copyright (c) 2006 Hypertriton, Inc.
 * <http://www.hypertriton.com/>
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
#include "and.h"

const ES_ComponentOps esAndOps = {
	{
		"ES_Component:ES_Digital:ES_And",
		sizeof(ES_And),
		{ 0,0 },
		ES_AndInit,
		NULL,			/* reinit */
		ES_ComponentDestroy,
		ES_ComponentLoad,
		ES_ComponentSave,
		ES_ComponentEdit
	},
	N_("AND Gate"),
	"And",
	ES_AndDraw,
	ES_AndEdit,
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};

const ES_Port esAndPinout[] = {
	{ 0, "",	0.0, 1.0 },
	{ 1, "Vcc",	1.0, -0.5 },
	{ 2, "Gnd",	1.0, +0.5 },
	{ 3, "A",	0.0, -0.5 },
	{ 4, "B",	0.0, +0.5 },
	{ 5, "A&B",	2.0, 0.0 },
	{ -1 },
};

void
ES_AndDraw(void *p, VG *vg)
{
	ES_And *gate = p;
	ES_Component *com = p;
	VG_Block *block;

	VG_Rectangle(vg, 0.25, -0.5625, 1.75, 0.5625);
	VG_Begin(vg, VG_LINES);
	VG_HLine(vg, 1.75, 2.00, 0.0);
	VG_HLine(vg, 0.00, 0.25, -0.5);
	VG_HLine(vg, 0.00, 0.25, +0.5);
	VG_End(vg);
#if 0
	VG_VLine(vg, 0.25, -0.625, +0.625);
#endif
	VG_Begin(vg, VG_TEXT);
	VG_SetStyle(vg, "component-name");
	VG_Vertex2(vg, 1.0, 0);
	VG_TextAlignment(vg, VG_ALIGN_MC);
	VG_Printf(vg, "&");
	VG_End(vg);
}

void
ES_AndInit(void *p, const char *name)
{
	ES_And *gate = p;

	ES_DigitalInit(gate, name, &esAndOps, esAndPinout);
	COM(gate)->intUpdate = ES_AndUpdate;
	ES_LogicOutput(gate, "A&B", ES_HI_Z);
}

void
ES_AndUpdate(void *p)
{
	ES_And *gate = p;

	if (ES_LogicInput(gate, "A") == ES_HIGH &&
	    ES_LogicInput(gate, "B") == ES_HIGH) {
		ES_LogicOutput(gate, "A&B", ES_HIGH);
	} else {
		ES_LogicOutput(gate, "A&B", ES_LOW);
	}
}

#ifdef EDITION
void *
ES_AndEdit(void *p)
{
	ES_And *gate = p;
	AG_Window *win, *wDig;
	AG_FSpinbutton *fsb;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;
	AG_Box *box;

	win = AG_WindowNew(0);

	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);
	ntab = AG_NotebookAddTab(nb, _("Digital"), AG_BOX_VERT);
	ES_DigitalEdit(gate, ntab);
	return (win);
}
#endif /* EDITION */
