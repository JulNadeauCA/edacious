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

#include <core/core.h>
#include "generic.h"

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
vBC(ES_NPN *u)
{
	return VPORT(u,PORT_B)-VPORT(u,PORT_C);
}

static M_Real
VbePrevStep(ES_NPN *u)
{
	return V_PREV_STEP(u,PORT_B)-V_PREV_STEP(u,PORT_E);
}

static M_Real
VbcPrevStep(ES_NPN *u)
{
	return V_PREV_STEP(u,PORT_B)-V_PREV_STEP(u,PORT_C);
}

static void 
ResetModel(ES_NPN *u)
{
	u->gPiF=1.0;
	u->gPiR=1.0;
	u->gmF=1.0;
	u->gmR=1.0;
	u->go=1.0;

	u->Ibf_eq = 0.0;
	u->Ibr_eq = 0.0;
	u->Icc_eq = 0.0;

	u->VbePrevIter = 0.7;
	u->VbcPrevIter = 0.7;
}

static void
UpdateModel(ES_NPN *u, ES_SimDC *dc, M_Real vBE, M_Real vBC)
{
	M_Real vCE = -vBC+vBE;

	M_Real VbeDiff = vBE - u->VbePrevIter;
	M_Real VbcDiff = vBC - u->VbcPrevIter;

	if (M_Fabs(VbeDiff) > u->Vt)
	{
		vBE = u->VbePrevIter + M_Fabs(VbeDiff)/VbeDiff*u->Vt;
		dc->isDamped = 1;
	}

	if (M_Fabs(VbcDiff) > u->Vt)
	{
		vBC = u->VbcPrevIter + M_Fabs(VbcDiff)/VbcDiff*u->Vt;
		dc->isDamped = 1;
	}

	u->VbePrevIter = vBE;
	u->VbcPrevIter = vBC;

	M_Real Ibf = (u->Ifs/u->betaF)*(M_Exp(vBE/u->Vt)-1);
	M_Real Ibr = (u->Irs/u->betaR)*(M_Exp(vBC/u->Vt)-1);

	M_Real Icc = (u->betaF*Ibf-u->betaR*Ibr)*(1+vCE/u->Va);

	u->gPiF = Ibf/u->Vt;
	u->gPiR = Ibr/u->Vt;

	u->go = Icc/u->Va;

	u->gmF = u->betaF*u->gPiF;
	u->gmR = u->betaR*u->gPiR;

    	u->Ibf_eq = Ibf - u->gPiF*vBE;
	u->Ibr_eq = Ibr - u->gPiR*vBC;
	u->Icc_eq = Icc - u->gmF*vBE + u->gmR*vBC - u->go*vCE;

	if (u->go < 1e-6)
		u->go=1e-6;
	if (u->gPiF < 1e-6)
		u->gPiF=1e-6;
	if (u->gPiR < 1e-6)
		u->gPiR=1e-6;
}

static void UpdateStamp(ES_NPN *u, ES_SimDC *dc)
{
	StampConductance(u->gPiF-u->gPiF_prev,u->sc_be);
	StampConductance(u->gPiR-u->gPiR_prev,u->sc_bc);

	StampConductance(u->go-u->goPrev,u->sc_ec);

	StampVCCS(u->gmF-u->gmF_prev,u->sv_bece);
	StampVCCS(u->gmR-u->gmR_prev,u->sv_bcec);

	StampCurrentSource(u->Ibf_eq-u->Ibf_eq_prev,u->si_eb);
	StampCurrentSource(u->Ibr_eq-u->Ibr_eq_prev,u->si_cb);
	StampCurrentSource(u->Icc_eq-u->Icc_eq_prev,u->si_ec);

	u->gPiF_prev = u->gPiF;
	u->gPiR_prev = u->gPiR;
	u->goPrev = u->go;
	u->gmF_prev = u->gmF;
	u->gmR_prev = u->gmR;
	u->Ibf_eq_prev = u->Ibf_eq;
	u->Ibr_eq_prev = u->Ibr_eq;
	u->Icc_eq_prev = u->Icc_eq;
}

static void Stamp(ES_NPN *u, ES_SimDC *dc)
{
	u->gPiF_prev = 0.0;
	u->gPiR_prev = 0.0;
	u->goPrev = 0.0;
	u->gmF_prev = 0.0;
	u->gmR_prev = 0.0;
	u->Ibf_eq_prev = 0.0;
	u->Ibr_eq_prev = 0.0;
	u->Icc_eq_prev = 0.0;

	UpdateStamp(u, dc);
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_NPN *u = obj;

	Uint b = PNODE(u,PORT_B);
	Uint e = PNODE(u,PORT_E);
	Uint c = PNODE(u,PORT_C);

	InitStampConductance(b,e, u->sc_be, dc);
	InitStampConductance(b,c, u->sc_bc, dc);

	InitStampConductance(e,c, u->sc_ec, dc);

	InitStampVCCS(b,e,c,e, u->sv_bece, dc);
	InitStampVCCS(b,c,e,c, u->sv_bcec, dc);

	InitStampCurrentSource(e,b, u->si_eb, dc);
	InitStampCurrentSource(c,b, u->si_cb, dc);
	InitStampCurrentSource(e,c, u->si_ec, dc);

	ResetModel(u);
	Stamp(u,dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_NPN *u = obj;

	if (dc->inputStep)
		ResetModel(u);
	else
	{
		u->VbePrevIter = VbePrevStep(u);
		u->VbcPrevIter = VbcPrevStep(u);
		UpdateModel(u,dc,VbePrevStep(u),VbcPrevStep(u));
	}

	Stamp(u,dc);

}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
        ES_NPN *u = obj;

	UpdateModel(u,dc,vBE(u),vBC(u));
	UpdateStamp(u,dc);
}

static void
Init(void *p)
{
	ES_NPN *u = p;

	ES_InitPorts(u, esNPNPorts);

	u->Vt = 0.025;
	u->Va = 10.0;
	u->betaF = 100.0;
	u->betaR = 1.0;
	u->Ifs = 1e-14;
	u->Irs = 1e-14;

	COMPONENT(u)->dcSimBegin = DC_SimBegin;
	COMPONENT(u)->dcStepBegin = DC_StepBegin;
	COMPONENT(u)->dcStepIter = DC_StepIter;
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_NPN *u = p;

	u->Vt = M_ReadReal(buf);
	u->Va = M_ReadReal(buf);
	u->betaF = M_ReadReal(buf);
	u->betaR = M_ReadReal(buf);
	u->Ifs = M_ReadReal(buf);
	u->Irs = M_ReadReal(buf);
	
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_NPN *u = p;

	M_WriteReal(buf, u->Vt);
	M_WriteReal(buf, u->Va);
	M_WriteReal(buf, u->betaF);
	M_WriteReal(buf, u->betaR);
	M_WriteReal(buf, u->Ifs);
	M_WriteReal(buf, u->Irs);

	return (0);
}

static void *
Edit(void *p)
{
	ES_NPN *u = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealPNZ(box, 0, "V", _("Threshold voltage: "), &u->Vt);
	M_NumericalNewRealPNZ(box, 0, "V", _("Early voltage: "), &u->Va);
	M_NumericalNewRealPNZ(box, 0, NULL, _("Forward Beta: "), &u->betaF);
	M_NumericalNewRealPNZ(box, 0, NULL, _("Reverse Beta: "), &u->betaR);
	M_NumericalNewRealPNZ(box, 0, "pA", _("Forward Sat. Current: "), &u->Ifs);
	M_NumericalNewRealPNZ(box, 0, "pA", _("Reverse Sat. Current: "), &u->Irs);

	return (box);
}

ES_ComponentClass esNPNClass = {
	{
		"ES_Component:ES_NPN",
		sizeof(ES_NPN),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,		/* load */
		Save,		/* save */
		Edit		/* edit */
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
