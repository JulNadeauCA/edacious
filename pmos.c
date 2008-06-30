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
 * Model for PMOS transistor
 */

#include <eda.h>
#include "pmos.h"

enum {
	PORT_G = 1,
	PORT_D = 2,
	PORT_S = 3
};
const ES_Port esPMOSPorts[] = {
	{ 0, "" },
	{ PORT_G, "G" },	/* Gate */
	{ PORT_D, "D" },	/* Drain */
	{ PORT_S, "S"},		/* Source */
	{ -1 },
};

static M_Real
vSD(ES_PMOS *u)
{
	return VPORT(u,PORT_S)-VPORT(u,PORT_D);
}

static M_Real
vSG(ES_PMOS *u)
{
	return VPORT(u,PORT_S)-VPORT(u,PORT_G);
}

static void
UpdateModel(ES_PMOS *u, M_Real vSG, M_Real vSD)
{
	M_Real I;

	if (vSG < u->Vt)
	{
		// Cutoff 

		I = 0;
		u->gm = 0;
		u->go = 0;

	}
	else if ((vSG-u->Vt) < vSD)
	{
		// Saturation

		I=(u->K)/2*(vSG-(u->Vt))*(vSG-(u->Vt));
		u->gm=sqrt(2*(u->K)*I);
		u->go=I/(u->Va);

	}
	else
	{
		// Triode
		
		I=(u->K)*((vSG-(u->Vt))-vSD/2)*vSD;
		u->gm=(u->K)*vSD;
		u->go=(u->K)*((vSG-(u->Vt))-vSD);

	}

	u->Ieq = I - u->gm*vSG - u->go*vSD;

}

static void
UpdateStamp(ES_PMOS *u, ES_SimDC *dc)
{
	Uint g = PNODE(u,PORT_G);
	Uint d = PNODE(u,PORT_D);
	Uint s = PNODE(u,PORT_S);

	StampVCCS(u->gm-u->gmPrev,s,g,s,d,dc->G);
	StampConductance(u->go-u->goPrev,s,d,dc->G);
	StampCurrentSource(u->Ieq-u->IeqPrev,d,s,dc->i);

	u->gmPrev = u->gm;
	u->goPrev = u->go;
	u->IeqPrev = u->Ieq;
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
	ES_PMOS *u = obj;

	UpdateModel(u,u->Vt+0.1,u->Vt+0.1);
	UpdateStamp(u,dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_PMOS *u = obj;

	UpdateModel(u,u->Vt+0.1,u->Vt+0.1);
	UpdateStamp(u,dc);

}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
        ES_PMOS *u = obj;

	UpdateModel(u,vSG(u),vSD(u));
	UpdateStamp(u,dc);
}

static void
Init(void *p)
{
	ES_PMOS *u = p;

	ES_InitPorts(u, esPMOSPorts);
	u->Vt = 0.5;
	u->Va = 10;
	u->K = 1e-3;

	u->gmPrev = 0.0;
	u->goPrev = 0.0;
	u->IeqPrev = 0.0;

	COMPONENT(u)->dcSimBegin = DC_SimBegin;
	COMPONENT(u)->dcStepBegin = DC_StepBegin;
	COMPONENT(u)->dcStepIter = DC_StepIter;
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_PMOS *u = p;

	u->Vt = M_ReadReal(buf);
	u->Va = M_ReadReal(buf);
	u->K = M_ReadReal(buf);
	
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_PMOS *u = p;

	M_WriteReal(buf, u->Vt);
	M_WriteReal(buf, u->Va);
	M_WriteReal(buf, u->K);

	return (0);
}

static void *
Edit(void *p)
{
	ES_PMOS *u = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealPNZ(box, 0, "V", _("Threshold voltage: "), &u->Vt);
	M_NumericalNewRealPNZ(box, 0, "V", _("Early voltage: "), &u->Va);
	M_NumericalNewRealPNZ(box, 0, NULL, _("K: "), &u->K);

	return (box);
}

ES_ComponentClass esPMOSClass = {
	{
		"ES_Component:ES_PMOS",
		sizeof(ES_PMOS),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,		/* load */
		Save,		/* save */
		Edit		/* edit */
	},
	N_("PMOS"),
	"U",
	"PMOS.eschem",
	"Generic|Nonlinear",
	&esIconPMOS,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
