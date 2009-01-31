/*
 * Copyright (c) 2008 Antoine Levitt (smeuuh@gmail.com)
 * Copyright (c) 2008 Steven Herbst (herbst@mit.edu)
 * Copyright (c) 2005-2009 Julien Nadeau (vedge@hypertriton.com)
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
 * Model for an independent (GUI) SPST switch.
 */

#include <core/core.h>
#include "generic.h"

const ES_Port esSpstPorts[] = {
	{ 0, "" },
	{ 1, "A" },
	{ 2, "B" },
	{ -1 },
};

static void
UpdateModel(ES_Spst *sw)
{
	sw->g = 1.0/(sw->state ? sw->Ron : sw->Roff);
}

static __inline__ void
Stamp(ES_Spst *sw)
{
	StampConductance(sw->g, sw->s);
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_Spst *sw = obj;
	Uint k = PNODE(sw,1);
	Uint j = PNODE(sw,2);

	InitStampConductance(k, j, sw->s, dc);
	UpdateModel(sw);
	Stamp(sw);
	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_Spst *sw = obj;

	UpdateModel(sw);
	Stamp(sw);
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_Spst *sw = obj;

	/* note that we do not call UpdateModel here; this is to ensure numerical stability if the user changes the switch state in the middle of a timestep */
	Stamp(sw);
}

static void
Init(void *p)
{
	ES_Spst *sw = p;

	ES_InitPorts(sw, esSpstPorts);
	sw->Ron = 1.0;
	sw->Roff = HUGE_VAL;
	sw->state = 0;

	COMPONENT(sw)->dcSimBegin = DC_SimBegin;
	COMPONENT(sw)->dcStepBegin = DC_StepBegin;
	COMPONENT(sw)->dcStepIter = DC_StepIter;

	M_BindReal(sw, "Ron", &sw->Ron);
	M_BindReal(sw, "Roff", &sw->Roff);
	AG_BindInt(sw, "state", &sw->state);
}

static double
Resistance(void *p, ES_Port *p1, ES_Port *p2)
{
	ES_Spst *sw = p;

	return (sw->state ? sw->Ron : sw->Roff);
}

static int
Export(void *p, enum circuit_format fmt, FILE *f)
{
	ES_Spst *sw = p;

	if (PNODE(sw,1) == -1 ||
	    PNODE(sw,2) == -1)
		return (0);
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		fprintf(f, "R%s %d %d %g\n", OBJECT(sw)->name,
		    PNODE(sw,1), PNODE(sw,2),
		    Resistance(sw, PORT(sw,1), PORT(sw,2)));
		break;
	}
	return (0);
}

static void
SwitchAll(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	int nstate = AG_INT(2);
	ES_Spst *spst;

	OBJECT_FOREACH_CLASS(spst, ckt, es_spst,
	    "ES_Circuit:ES_Component:ES_Spst:*") {
		spst->state = (nstate == -1) ? !spst->state : nstate;
	}
}

static void
ToggleState(AG_Event *event)
{
	ES_Spst *sw = AG_PTR(1);

	sw->state = (sw->state == 1) ? 0 : 1;
}

static void
ClassMenu(ES_Circuit *ckt, AG_MenuItem *m)
{
	AG_MenuAction(m, _("Switch on all"), NULL,
	    SwitchAll, "%p,%i", ckt, 1);
	AG_MenuAction(m, _("Switch off all"), NULL,
	    SwitchAll, "%p,%i", ckt, 0);
	AG_MenuAction(m, _("Toggle all"), NULL,
	    SwitchAll, "%p,%i", ckt, -1);
}

static void
InstanceMenu(void *p, AG_MenuItem *m)
{
	ES_Spst *spst = p;

	AG_MenuIntBool(m, _("Toggle state"), NULL, &spst->state, 0);
}

static void *
Edit(void *p)
{
	ES_Spst *sw = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealPNZ(box, 0, "ohm", _("ON resistance: "), &sw->Ron);
	M_NumericalNewRealPNZ(box, 0, "ohm", _("OFF resistance: "), &sw->Roff);
	AG_ButtonAct(box, AG_BUTTON_EXPAND, _("Toggle state"),
	    ToggleState, "%p", sw);
	return (box);
}

ES_ComponentClass esSpstClass = {
	{
		"Edacious(Circuit:Component:Spst)"
		"@generic",
		sizeof(ES_Spst),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		Edit
	},
	N_("Switch (SPST)"),
	"Sw",
	"Generic|Switches|User Interface",
	&esIconSPST,
	NULL,			/* draw */
	InstanceMenu,
	ClassMenu,
	Export,
	NULL			/* connect */
};
