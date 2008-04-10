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
 * Model for the resistor.
 */

#include <eda.h>
#include "resistor.h"

const ES_Port esResistorPinout[] = {
	{ 0, "",  {0.000, 0.625} },
	{ 1, "A", {0.000, 0.000} },
	{ 2, "B", {1.250, 0.000} },
	{ -1 },
};

#if 0
static void
Draw(void *p, VG_Node *vn)
{
	ES_Resistor *r = p;
	VG_Point *p1, *p2, *p3, *p4;
	VG_Point *pTL, *pTR, *pBL, *pBR;
	VG_Text *vt;

	p1 = VG_PointNew(vn, VGVECTOR(0.00000f, 0.0f));
	p2 = VG_PointNew(vn, VGVECTOR(0.15600f, 0.0f));
	p3 = VG_PointNew(vn, VGVECTOR(1.25000f, 0.0f));
	p4 = VG_PointNew(vn, VGVECTOR(0.09375f, 0.0f));
	pTL = VG_PointNew(vn, VGVECTOR(0.15600f, -0.240f));
	pTR = VG_PointNew(vn, VGVECTOR(1.09375f, -0.240f));
	pBL = VG_PointNew(vn, VGVECTOR(0.15600f, +0.240f));
	pBR = VG_PointNew(vn, VGVECTOR(1.09375f, +0.240f));

	VG_LineNew(vn, p1,p2);	 VG_LineNew(vn, p3,p4);
	VG_LineNew(vn, p2,pTL);	 VG_LineNew(vn, p2,pBL);
	VG_LineNew(vn, p3,pTR);	 VG_LineNew(vn, p3,pBR);
	VG_LineNew(vn, pTL,pTR); VG_LineNew(vn, pBL,pBR);

	p1 = VG_PointNew(vn, VGVECTOR(0.625f, 0.000f));
	vt = VG_TextNew(vn, p1);
	VG_TextAlignment(vt, VG_ALIGN_MC);
	VG_TextPrintf(vt, "%s", OBJECT(r)->name);
}
#endif

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Resistor *r = p;

	r->resistance = AG_ReadDouble(buf);
	r->tolerance = (int)AG_ReadUint32(buf);
	r->power_rating = AG_ReadDouble(buf);
	r->Tc1 = AG_ReadFloat(buf);
	r->Tc2 = AG_ReadFloat(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Resistor *r = p;

	AG_WriteDouble(buf, r->resistance);
	AG_WriteUint32(buf, (Uint32)r->tolerance);
	AG_WriteDouble(buf, r->power_rating);
	AG_WriteFloat(buf, r->Tc1);
	AG_WriteFloat(buf, r->Tc2);
	return (0);
}

static int
Export(void *p, enum circuit_format fmt, FILE *f)
{
	ES_Resistor *r = p;

	if (PNODE(r,1) == -1 ||
	    PNODE(r,2) == -1)
		return (0);
	
	switch (fmt) {
	case CIRCUIT_SPICE3:
		fprintf(f, "%s %d %d %g\n", OBJECT(r)->name,
		    PNODE(r,1), PNODE(r,2), r->resistance);
		break;
	}
	return (0);
}

#if 0
static SC_Real
Resistance(void *p, ES_Port *p1, ES_Port *p2)
{
	ES_Resistor *r = p;
	SC_Real deltaT = COM_T0 - COM(r)->Tspec;

	return (r->resistance * (1.0 + r->Tc1*deltaT + r->Tc2*deltaT*deltaT));
}
#endif

/*
 * Load the element into the conductance matrix. All conductances between
 * (k,j) are added to (k,k) and (j,j), and subtracted from (k,j) and (j,k).
 *
 *   |  Vk    Vj  | RHS
 *   |------------|-----
 * k |  Gkj  -Gkj |
 * j | -Gkj   Gkj |
 *   |------------|-----
 */
static int
LoadDC_G(void *p, SC_Matrix *G)
{
	ES_Resistor *r = p;
	ES_Node *n;
	u_int k = PNODE(r,1);
	u_int j = PNODE(r,2);
	SC_Real g;

	if (r->resistance == 0.0 || k == -1 || j == -1 || (k == 0 && j == 0)) {
		AG_SetError("null resistance");
		return (-1);
	}
	g = 1.0/r->resistance;
	if (k != 0) {
		G->mat[k][k] += g;
	}
	if (j != 0) {
		G->mat[j][j] += g;
	}
	if (k != 0 && j != 0) {
		G->mat[k][j] -= g;
		G->mat[j][k] -= g;
	}
	return (0);
}

static int
LoadSP(void *p, SC_Matrix *S, SC_Matrix *N)
{
	ES_Resistor *res = p;
	u_int k = PNODE(res,1);
	u_int j = PNODE(res,2);
	SC_Real r, z, f;
	
	if (res->resistance == 0.0 ||
	    k == -1 || j == -1 || (k == 0 && j == 0)) {
		AG_SetError("null resistance");
		return (-1);
	}

	r = res->resistance;
	z = r/COM_Z0;
	S->mat[k][k] += z/(z+2);
	S->mat[j][j] += z/(z+2);
	S->mat[k][j] -= z/(z+2);
	S->mat[j][k] -= z/(z+2);

	f = COM(res)->Tspec*4.0*r*COM_Z0 /
	    ((2.0*COM_Z0+r)*(2.0*COM_Z0+r)) / COM_T0;
	N->mat[k][k] += f;
	N->mat[j][j] += f;
	N->mat[k][j] -= f;
	N->mat[j][k] -= f;
	return (0);
}

static void
Init(void *p)
{
	ES_Resistor *r = p;

	ES_ComponentSetPorts(r, esResistorPinout);
	r->resistance = 1;
	r->power_rating = HUGE_VAL;
	r->tolerance = 0;
	COM(r)->loadDC_G = LoadDC_G;
	COM(r)->loadSP = LoadSP;
}

static void *
Edit(void *p)
{
	ES_Resistor *r = p;
	AG_Window *win;

	win = AG_WindowNew(0);
	AG_NumericalNewDblR(win, 0, "ohm", _("Resistance: "), &r->resistance,
	    1.0, HUGE_VAL);
	AG_NumericalNewIntR(win, 0, _("Tolerance: "), "%", &r->tolerance,
	    1, 100);
	AG_NumericalNewDblR(win, 0, "W", _("Power rating: "), &r->power_rating,
	    0.0, HUGE_VAL);

	AG_NumericalNewFlt(win, 0, "mohm/degC", _("Temp. coeff.: "), &r->Tc1);
	AG_NumericalNewFlt(win, 0, "mohm/degC^2", _("Temp. coeff.: "), &r->Tc2);
	return (win);
}

ES_ComponentClass esResistorClass = {
	{
		"ES_Component:ES_Resistor",
		sizeof(ES_Resistor),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Resistor"),
	"R",
	"Resistor.vg",
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	Export,
	NULL			/* connect */
};
