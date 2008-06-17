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
 * Model for an ideal digital inverter.
 */

#include <eda.h>
#include "inverter.h"

const ES_Port esInverterPorts[] = {
	{ 0, "" },
	{ 1, "Vcc" },
	{ 2, "Gnd" },
	{ 3, "A" },
	{ 4, "A-bar" },
	{ -1 },
};

static void
Init(void *p)
{
	ES_Inverter *inv = p;

	ES_InitPorts(inv, esInverterPorts);
	COM(inv)->intUpdate = ES_InverterUpdate;
#if 0
	ES_SetSpec(inv, "Tp", _("Propagation delay from A to A-bar"),
	    "Vcc=5;Tamb=25",	0, 50, 90,
	    "Vcc=10;Tamb=25",	0, 30, 60,
	    "Vcc=15;Tamb=25",	0, 25, 50,
	    NULL);
	ES_SetSpec(inv, "Tt", _("Transition time for A-bar"),
	    "Vcc=5;Tamb=25",	0, 80, 150,
	    "Vcc=10;Tamb=25",	0, 50, 100,
	    "Vcc=15;Tamb=25",	0, 40, 80,
	    NULL);
	ES_SetSpec(inv, "Cin", _("Input capacitance"),
	    "Tamb=25", 0, 6, 15,
	    NULL);
	ES_SetSpec(inv, "Cpd", _("Power dissipation capacitance"),
	    "Tamb=25", 0, 6, 15,
	    NULL);
	ES_SetSpec(inv, "Idd", _("Quiescent device current"),
	    "Vcc=5;Tamb=-55",	0, 0, 0.25,
	    "Vcc=10;Tamb=-55",	0, 0, 0.5,
	    "Vcc=15;Tamb=-55",	0, 0, 1.0,
	    "Vcc=5;Tamb=25",	0, 0, 0.25,
	    "Vcc=10;Tamb=25",	0, 0, 0.5,
	    "Vcc=15;Tamb=25",	0, 0, 1.0,
	    "Vcc=5;Tamb=125",	0, 0, 7.5,
	    "Vcc=10;Tamb=125",	0, 0, 15.0,
	    "Vcc=15;Tamb=125",	0, 0, 30.0,
	    NULL);
#endif
	ES_LogicOutput(inv, "A-bar", ES_HI_Z);
}

void
ES_InverterUpdate(void *p)
{
	ES_Inverter *inv = p;

	switch (ES_LogicInput(inv, "A")) {
	case ES_HIGH:
		ES_LogicOutput(inv, "A-bar", ES_LOW);
		break;
	case ES_LOW:
		ES_LogicOutput(inv, "A-bar", ES_HIGH);
		break;
	}
}

ES_ComponentClass esInverterClass = {
	{
		"ES_Component:ES_Digital:ES_Inverter",
		sizeof(ES_Inverter),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		NULL		/* edit */
	},
	N_("Inverter"),
	"Inv",
	"Digital/Inverter.eschem",
	"Generic|Digital|Digital/Gates",
	&esIconInverter,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
