/*
 * Copyright (c) 2009 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Connection point in PCB layout. Mainly useful for the editor, LayoutNode
 * entities are usually associated with entities such as Pad, Pin and Via.
 */

#include "core.h"
#include <agar/gui/primitive.h>
#include <agar/core/limits.h>

ES_LayoutNode *
ES_LayoutNodeNew(void *pNode)
{
	ES_LayoutNode *ln;

	ln = AG_Malloc(sizeof(ES_LayoutNode));
	VG_NodeInit(ln, &esLayoutNodeOps);
	VG_NodeAttach(pNode, ln);
	return (ln);
}

static void
Init(void *p)
{
	ES_LayoutNode *ln = p;

	ln->flags = 0;
	VG_SetColorRGB(ln, 0, 150, 0);
}

static int
Load(void *p, AG_DataSource *ds, const AG_Version *ver)
{
	ES_LayoutNode *ln = p;

	ln->flags = (Uint)AG_ReadUint8(ds);
	return (0);
}

static void
Save(void *p, AG_DataSource *ds)
{
	ES_LayoutNode *ln = p;

	AG_WriteUint8(ds, (Uint8)ln->flags);
}

static void
Draw(void *p, VG_View *vv)
{
	ES_LayoutNode *ln = p;
	AG_Color c;
	int x, y;

	VG_GetViewCoords(vv, VG_Pos(ln), &x, &y);
	c = VG_MapColorRGB(VGNODE(ln)->color);
	AG_DrawCircle(vv, x, y, (int)(1.0f*vv->scale), &c);
}

static void
Extent(void *p, VG_View *vv, VG_Vector *a, VG_Vector *b)
{
	ES_LayoutNode *ln = p;
	VG_Vector vPos = VG_Pos(ln);

	a->x = vPos.x - 1.0f;
	a->y = vPos.y - 1.0f;
	b->x = vPos.x + 1.0f;
	b->y = vPos.y + 1.0f;
}

static float
PointProximity(void *p, VG_View *vv, VG_Vector *vPt)
{
	ES_LayoutNode *ln = p;
	VG_Vector pos = VG_Pos(ln);
	float d;

	d = VG_Distance(pos, *vPt);
	*vPt = pos;
	return (d);
}

static void
Move(void *p, VG_Vector vCurs, VG_Vector vRel)
{
	ES_LayoutNode *ln = p;

	VG_Translate(ln, vRel);
}

VG_NodeOps esLayoutNodeOps = {
	N_("LayoutNode"),
	&esIconPortEditor,
	sizeof(ES_LayoutNode),
	Init,
	NULL,			/* destroy */
	Load,
	Save,
	Draw,
	Extent,
	PointProximity,
	NULL,			/* lineProximity */
	NULL,			/* delete */
	Move,
	NULL,			/* edit */
};
