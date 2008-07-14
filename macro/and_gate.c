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
 * Model for the ideal digital AND gate.
 */

#include <core/core.h>
#include "macro.h"

const ES_Port esAndPorts[] = {
	{ 0, ""	},
	{ 1, "Vcc" },
	{ 2, "Gnd" },
	{ 3, "A" },
	{ 4, "B" },
	{ 5, "A&B" },
	{ -1 },
};

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_LogicOutput(obj, "A&B", ES_HI_Z);
	return (0);
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_And *gate = obj;

	if (ES_LogicInput(gate, "A") == ES_HIGH &&
	    ES_LogicInput(gate, "B") == ES_HIGH) {
		ES_LogicOutput(gate, "A&B", ES_HIGH);
	} else {
		ES_LogicOutput(gate, "A&B", ES_LOW);
	}
	ES_DigitalStepIter(gate, dc);
}

static void
Init(void *p)
{
	ES_And *gate = p;

	ES_DigitalInitPorts(gate, esAndPorts);
	COMPONENT(gate)->dcSimBegin = DC_SimBegin;
	COMPONENT(gate)->dcStepIter = DC_StepIter;
}

ES_ComponentClass esAndClass = {
	{
		"ES_Component:ES_Digital:ES_And",
		sizeof(ES_And),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		NULL		/* edit */
	},
	N_("AND Gate"),
	"And",
	"Digital/AndGate.eschem",
	"Generic|Digital|Digital/Gates",
	&esIconANDGate,
	NULL,		/* draw */
	NULL,		/* instance_menu */
	NULL,		/* class_menu */
	NULL,		/* export */
	NULL		/* connect */
};
