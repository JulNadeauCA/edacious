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
 * Model for NMOS transistor
 */

#include <eda.h>
#include "nmos.h"

enum {
	PORT_G = 1,
	PORT_D = 2,
	PORT_S = 3
};
const ES_Port esNMOSPorts[] = {
	{ 0, "" },
	{ PORT_G, "G" },	/* Gate */
	{ PORT_D, "D" },	/* Drain */
	{ PORT_S, "S"},		/* Source */
	{ -1 },
};

static M_Real
NMOS_vDS(ES_NMOS *u)
{
	return VPORT(u,PORT_D)-VPORT(u,PORT_S);
}

static M_Real
NMOS_vGS(ES_NMOS *u)
{
	return VPORT(u,PORT_G)-VPORT(u,PORT_S);
}

static void
UpdateNMOSModel(ES_NMOS *u, M_Real vGS, M_Real vDS)
{
	u->IeqPrev = u->Ieq;
	u->gmPrev = u->gm;
	u->goPrev = u->go;

	M_Real I;

	if (vGS < u->Vt)
	{
		// Cutoff 

		I = 0;
		u->gm = 0;
		u->go = 0;

	}
	else if ((vGS-u->Vt) < vDS)
	{
		// Saturation

		I=(u->K)/2*(vGS-(u->Vt))*(vGS-(u->Vt));
		u->gm=sqrt(2*(u->K)*I);
		u->go=I/(u->Va);

	}
	else
	{
		// Triode
		
		I=(u->K)*((vGS-(u->Vt))-vDS/2)*vDS;
		u->gm=(u->K)*vDS;
		u->go=(u->K)*((vGS-(u->Vt))-vDS);

	}

	u->Ieq = I - u->gm*vGS - u->go*vDS;

}

/*
 * Load the element into the conductance matrix. All conductances between
 * (k,j) are added to (k,k) and (j,j), and subtracted from (k,j) and (j,k).
 *
 *   |  Vk    Vj  | RHS
 *   |------------|-----
 * k |  g    -g   |  I
 * j | -g     g   | -I
 *   |------------|-----
 */

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_NMOS *u = obj;

	Uint g = PNODE(u,PORT_G);
	Uint d = PNODE(u,PORT_D);
	Uint s = PNODE(u,PORT_S);

	M_Real vGS_Guess = 0.0;
	M_Real vDS_Guess = 0.0;
	UpdateNMOSModel(u,vGS_Guess,vDS_Guess);

	StampVCCS(u->gm,g,s,d,s,dc->G);
	StampConductance(u->go,d,s,dc->G);
	StampCurrentSource(u->Ieq,s,d,dc->i);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_NMOS *u = obj;

	Uint g = PNODE(u,PORT_G);
	Uint d = PNODE(u,PORT_D);
	Uint s = PNODE(u,PORT_S);

	M_Real vGS_Guess = 0;
	M_Real vDS_Guess = 0;
	UpdateNMOSModel(u,vGS_Guess,vDS_Guess);

	StampVCCS(u->gm-u->gmPrev,g,s,d,s,dc->G);
	StampConductance(u->go-u->goPrev,d,s,dc->G);
	StampCurrentSource(u->Ieq-u->IeqPrev,s,d,dc->i);
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
        ES_NMOS *u = obj;

	Uint g = PNODE(u,PORT_G);
	Uint d = PNODE(u,PORT_D);
	Uint s = PNODE(u,PORT_S);

	UpdateNMOSModel(u,NMOS_vGS(u),NMOS_vDS(u));

	StampVCCS(u->gm-u->gmPrev,g,s,d,s,dc->G);
	StampConductance(u->go-u->goPrev,d,s,dc->G);
	StampCurrentSource(u->Ieq-u->IeqPrev,s,d,dc->i);
}

static void
Init(void *p)
{
	ES_NMOS *u = p;

	ES_InitPorts(u, esNMOSPorts);
	u->Vt = 0.5;
	u->Va = 10;
	u->K = 1e-3;

	COMPONENT(u)->dcSimBegin = DC_SimBegin;
	COMPONENT(u)->dcStepBegin = DC_StepBegin;
	COMPONENT(u)->dcStepIter = DC_StepIter;
}

ES_ComponentClass esNMOSClass = {
	{
		"ES_Component:ES_NMOS",
		sizeof(ES_NMOS),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		NULL		/* edit */
	},
	N_("NMOS"),
	"U",
	"NMOS.eschem",
	"Generic|Nonlinear",
	NULL,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
