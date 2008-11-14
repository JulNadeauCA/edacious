/*
 * Copyright (c) 2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Wire entity. Displays connections between ports.
 */

#include "core.h"
#include <agar/gui/primitive.h>
#include <agar/core/limits.h>

ES_SchemWire *
ES_SchemWireNew(void *pNode, void *p1, void *p2)
{
	ES_SchemWire *sw;

	sw = AG_Malloc(sizeof(ES_SchemWire));
	VG_NodeInit(sw, &esSchemWireOps);
	sw->p1 = VGNODE(p1);
	sw->p2 = VGNODE(p2);

	VG_NodeAttach(pNode, sw);
	VG_AddRef(sw, sw->p1);
	VG_AddRef(sw, sw->p2);
	return (sw);
}

static void
Init(void *p)
{
	ES_SchemWire *sw = p;

	sw->p1 = NULL;
	sw->p2 = NULL;
	sw->thickness = 1;
	sw->name[0] = '\0';
	sw->wire = NULL;
}

static int
Load(void *p, AG_DataSource *ds, const AG_Version *ver)
{
	ES_SchemWire *sw = p;

	AG_CopyString(sw->name, ds, sizeof(sw->name));
	if ((sw->p1 = VG_ReadRef(ds, sw, "SchemPort")) == NULL ||
	    (sw->p2 = VG_ReadRef(ds, sw, "SchemPort")) == NULL) {
		return (-1);
	}
	sw->thickness = (Uint)AG_ReadUint32(ds);
	return (0);
}

static void
Save(void *p, AG_DataSource *ds)
{
	ES_SchemWire *sw = p;

	AG_WriteString(ds, OBJECT(sw->wire)->name);
	VG_WriteRef(ds, sw->p1);
	VG_WriteRef(ds, sw->p2);
	AG_WriteUint32(ds, (Uint32)sw->thickness);
}

static void
Draw(void *p, VG_View *vv)
{
	ES_SchemWire *sw = p;
	VG_Vector v1 = VG_Pos(sw->p1);
	VG_Vector v2 = VG_Pos(sw->p2);
	int x1, y1, x2, y2;
	
	VG_GetViewCoords(vv, v1, &x1, &y1);
	VG_GetViewCoords(vv, v2, &x2, &y2);
	AG_DrawLine(vv, x1,y1, x2,y2, VG_MapColorRGB(VGNODE(sw)->color));
}

static void
Extent(void *p, VG_View *vv, VG_Vector *a, VG_Vector *b)
{
	ES_SchemWire *sw = p;
	VG_Vector p1, p2;

	p1 = VG_Pos(sw->p1);
	p2 = VG_Pos(sw->p2);
	a->x = MIN(p1.x, p2.x);
	a->y = MIN(p1.y, p2.y);
	b->x = MAX(p1.x, p2.x);
	b->y = MAX(p1.y, p2.y);
}

static float
PointProximity(void *p, VG_View *vv, VG_Vector *vPt)
{
	ES_SchemWire *sw = p;
	VG_Vector v1 = VG_Pos(sw->p1);
	VG_Vector v2 = VG_Pos(sw->p2);

	return VG_PointLineDistance(v1, v2, vPt);
}

static void
Delete(void *p)
{
	ES_SchemWire *sw = p;

	if (VG_DelRef(sw, sw->p1) == 0) { VG_Delete(sw->p1); }
	if (VG_DelRef(sw, sw->p2) == 0) { VG_Delete(sw->p2); }
}

static void
Move(void *p, VG_Vector vCurs, VG_Vector vRel)
{
	ES_SchemWire *sw = p;
	VG_Matrix T1, T2;

	T1 = sw->p1->T;
	T2 = sw->p2->T;
	VG_LoadIdentity(sw->p1);
	VG_LoadIdentity(sw->p2);
	VG_Translate(sw->p1, vRel);
	VG_Translate(sw->p2, vRel);
	VG_MultMatrix(&sw->p1->T, &T1);
	VG_MultMatrix(&sw->p2->T, &T2);
}

VG_NodeOps esSchemWireOps = {
	N_("SchemWire"),
	&esIconInsertWire,
	sizeof(ES_SchemWire),
	Init,
	NULL,			/* destroy */
	Load,
	Save,
	Draw,
	Extent,
	PointProximity,
	NULL,			/* lineProximity */
	Delete,
	Move
};
