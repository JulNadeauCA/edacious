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
 * Model for NPN transistor
 */

#include <eda.h>
#include "npn.h"

enum {
	PORT_B = 1,
	PORT_E = 2,
	PORT_C = 3
};
const ES_Port esNPNPorts[] = {
	{ 0, "" },
	{ PORT_B, "B" },	/* Base */
	{ PORT_E, "E" },	/* Emitter */
	{ PORT_C, "C"},		/* Collector */
	{ -1 },
};

static M_Real
vBE(ES_NPN *u)
{
	return VPORT(u,PORT_B)-VPORT(u,PORT_E);
}

static M_Real
vCE(ES_NPN *u)
{
	return VPORT(u,PORT_C)-VPORT(u,PORT_E);
}

static void
UpdateModel(ES_NPN *u, ES_SimDC *dc, M_Real vBE, M_Real vCE)
{
	M_Real VbeDiff = vBE - u->VbePrev;

	if (M_Fabs(VbeDiff) > u->Vt)
	{
		vBE = u->VbePrev + M_Fabs(VbeDiff)/VbeDiff*vBE;
		dc->isDamped = 1;
	}
	
	u->VbePrev = vBE;
	u->VcePrev = vCE;

	M_Real IE = u->Ies*(M_Exp(vBE/u->Vt)-1.0);
	M_Real IC = u->alpha*IE;
	M_Real IB = (1-u->alpha)*IE;

	u->gm = IC/u->Vt;
	u->go = IC/u->Va;
	u->gPi = u->gm * (1-u->alpha)/u->alpha;


	if (u->gPi < 0.01)
	{
		u->gPi = 0.01;
	}
	if (u->gm < 0.01)
	{
		u->gm = 0.01;
	}
	if (u->go < 0.01)
	{
		u->go = 0.01;
	}

	u->IEeq = IE - u->gm*vBE - u->go*vBE;
	u->IBeq = IB - u->gPi*vBE;

}

static void UpdateStamp(ES_NPN *u, ES_SimDC *dc)
{
	Uint b = PNODE(u,PORT_B);
	Uint e = PNODE(u,PORT_E);
	Uint c = PNODE(u,PORT_C);

	StampConductance(u->gPi-u->gPiPrev,b,e,dc->G);
	StampCurrentSource(u->IBeq-u->IBeqPrev,b,e,dc->G);

	StampVCCS(u->gm-u->gmPrev,b,e,c,e,dc->G);
	StampConductance(u->go-u->goPrev,c,e,dc->G);
	StampCurrentSource(u->IEeq-u->IEeqPrev,e,c,dc->i);

	u->gPiPrev = u->gPi;
	u->gmPrev = u->gm;
	u->goPrev = u->go;
	u->IEeqPrev = u->IEeq;
	u->IBeqPrev = u->IBeq;
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_NPN *u = obj;

	u->VbePrev = 0.7;
	u->VcePrev = 0.7;
	UpdateModel(u,dc,u->VbePrev,u->VcePrev);
	UpdateStamp(u,dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_NPN *u = obj;

	UpdateModel(u,dc,vBE(u),vCE(u));
	UpdateStamp(u,dc);

}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
        ES_NPN *u = obj;

	UpdateModel(u,dc,vBE(u),vCE(u));
	UpdateStamp(u,dc);
}

static void
Init(void *p)
{
	ES_NPN *u = p;

	ES_InitPorts(u, esNPNPorts);
	u->Vt = 0.5;
	u->alpha = 0.99;
	u->Ies = 1e-14;

	u->gPiPrev = 0.0;
	u->gmPrev = 0.0;
	u->goPrev = 0.0;
	u->IEeqPrev = 0.0;
	u->IBeqPrev = 0.0;

	COMPONENT(u)->dcSimBegin = DC_SimBegin;
	COMPONENT(u)->dcStepBegin = DC_StepBegin;
	COMPONENT(u)->dcStepIter = DC_StepIter;
}

ES_ComponentClass esNPNClass = {
	{
		"ES_Component:ES_NPN",
		sizeof(ES_NPN),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		NULL		/* edit */
	},
	N_("NPN"),
	"U",
	"NPN.eschem",
	"Generic|Nonlinear",
	&esIconNPN,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
