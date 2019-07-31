/*
 * Copyright (c) 2008-2019 Julien Nadeau Carriere <vedge@csoft.net>
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
 * Conductive trace between two LayoutNodes.
 */

#include "core.h"
#include <agar/gui/primitive.h>

ES_LayoutTrace *
ES_LayoutTraceNew(void *pNode, void *p1, void *p2)
{
	ES_LayoutTrace *lt;

	lt = AG_Malloc(sizeof(ES_LayoutTrace));
	VG_NodeInit(lt, &esLayoutTraceOps);
	lt->p1 = VGNODE(p1);
	lt->p2 = VGNODE(p2);

	VG_NodeAttach(pNode, lt);
	VG_AddRef(lt, lt->p1);
	VG_AddRef(lt, lt->p2);
	return (lt);
}

static void
Init(void *p)
{
	ES_LayoutTrace *lt = p;

	lt->flags = 0;
	lt->p1 = NULL;
	lt->p2 = NULL;
	lt->thickness = 1.0f;
	lt->clearance = 1.0f;
}

static int
Load(void *p, AG_DataSource *ds, const AG_Version *ver)
{
	ES_LayoutTrace *lt = p;

	if ((lt->p1 = VG_ReadRef(ds, lt, "LayoutNode")) == NULL ||
	    (lt->p2 = VG_ReadRef(ds, lt, "LayoutNode")) == NULL) {
		return (-1);
	}
	lt->thickness = AG_ReadFloat(ds);
	lt->clearance = AG_ReadFloat(ds);
	return (0);
}

static void
Save(void *p, AG_DataSource *ds)
{
	ES_LayoutTrace *lt = p;

	VG_WriteRef(ds, lt->p1);
	VG_WriteRef(ds, lt->p2);
	AG_WriteFloat(ds, lt->thickness);
	AG_WriteFloat(ds, lt->clearance);
}

static void
Draw(void *p, VG_View *vv)
{
	ES_LayoutTrace *lt = p;
	VG_Vector v1 = VG_Pos(lt->p1);
	VG_Vector v2 = VG_Pos(lt->p2);
	AG_Color c;
	int x1, y1, x2, y2;
	
	VG_GetViewCoords(vv, v1, &x1, &y1);
	VG_GetViewCoords(vv, v2, &x2, &y2);
	c = VG_MapColorRGB(VGNODE(lt)->color);
	AG_DrawLine(vv, x1,y1, x2,y2, &c);
}

static void
Extent(void *p, VG_View *vv, VG_Vector *a, VG_Vector *b)
{
	ES_LayoutTrace *lt = p;
	VG_Vector p1, p2;

	p1 = VG_Pos(lt->p1);
	p2 = VG_Pos(lt->p2);
	a->x = MIN(p1.x, p2.x);
	a->y = MIN(p1.y, p2.y);
	b->x = MAX(p1.x, p2.x);
	b->y = MAX(p1.y, p2.y);
}

static float
PointProximity(void *p, VG_View *vv, VG_Vector *vPt)
{
	ES_LayoutTrace *lt = p;
	VG_Vector v1 = VG_Pos(lt->p1);
	VG_Vector v2 = VG_Pos(lt->p2);

	return VG_PointLineDistance(v1, v2, vPt);
}

static void
Delete(void *p)
{
	ES_LayoutTrace *lt = p;

	if (VG_DelRef(lt, lt->p1) == 0) { VG_Delete(lt->p1); }
	if (VG_DelRef(lt, lt->p2) == 0) { VG_Delete(lt->p2); }
}

static void
Move(void *p, VG_Vector vCurs, VG_Vector vRel)
{
	ES_LayoutTrace *lt = p;
	VG_Matrix T1, T2;

	T1 = lt->p1->T;
	T2 = lt->p2->T;
	VG_LoadIdentity(lt->p1);
	VG_LoadIdentity(lt->p2);
	VG_Translate(lt->p1, vRel);
	VG_Translate(lt->p2, vRel);
	VG_MultMatrix(&lt->p1->T, &T1);
	VG_MultMatrix(&lt->p2->T, &T2);
}

static void *
Edit(void *p, VG_View *vv)
{
	ES_LayoutTrace *lt = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	AG_NumericalNewFlt(box, 0, "mm", _("Thickness: "), &lt->thickness);
	AG_NumericalNewFlt(box, 0, "mm", _("Clearance: "), &lt->clearance);
	AG_CheckboxNewFlag(box, 0, _("Rat line"), &lt->flags, ES_LAYOUTTRACE_RAT);

	return (box);
}

VG_NodeOps esLayoutTraceOps = {
	N_("LayoutTrace"),
	&esIconInsertWire,
	sizeof(ES_LayoutTrace),
	Init,
	NULL,			/* destroy */
	Load,
	Save,
	Draw,
	Extent,
	PointProximity,
	NULL,			/* lineProximity */
	Delete,
	Move,
	Edit
};
