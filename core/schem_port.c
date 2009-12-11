/*
 * Copyright (c) 2008-2009 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Port entity. Defines a connection point in the schematic.
 */

#include "core.h"
#include <agar/gui/primitive.h>
#include <agar/gui/opengl.h>
#include <agar/core/limits.h>

ES_SchemPort *
ES_SchemPortNew(void *pNode, ES_Port *port)
{
	ES_SchemPort *sp;

	sp = AG_Malloc(sizeof(ES_SchemPort));
	VG_NodeInit(sp, &esSchemPortOps);
	VG_NodeAttach(pNode, sp);
	sp->port = port;
	return (sp);
}

static void
Init(void *p)
{
	ES_SchemPort *sp = p;

	sp->flags = 0;
	sp->name[0] = '\0';
	sp->r = 1.2f;
	sp->comName[0] = '\0';
	sp->port = NULL;
	sp->portName = -1;
	VG_SetColorRGB(sp, 0, 150, 0);
}

static int
Load(void *p, AG_DataSource *ds, const AG_Version *ver)
{
	ES_SchemPort *sp = p;

	sp->flags = (Uint)AG_ReadUint8(ds);
	AG_CopyString(sp->name, ds, sizeof(sp->name));
	AG_CopyString(sp->comName, ds, sizeof(sp->comName));
	sp->portName = (int)AG_ReadUint32(ds);
	return (0);
}

static void
Save(void *p, AG_DataSource *ds)
{
	ES_SchemPort *sp = p;

	AG_WriteUint8(ds, (Uint8)sp->flags);
	AG_WriteString(ds, sp->name);
	AG_WriteString(ds, (sp->port != NULL && sp->port->com != NULL) ?
	                   OBJECT(sp->port->com)->name : "");
	AG_WriteUint32(ds, (sp->port != NULL) ? (Uint32)sp->port->n : 0);
}

static void
Draw(void *p, VG_View *vv)
{
	ES_SchemPort *sp = p;
	char text[16];
	int x, y;
	float r;

	VG_GetViewCoords(vv, VG_Pos(sp), &x, &y);
	if (sp->port == NULL || sp->port->com == NULL ||
	    sp->port->com->ckt->flags & ES_CIRCUIT_SHOW_NODES) {
		if (sp->port != NULL && sp->port->flags & ES_PORT_SELECTED) {
			r = sp->r+1.0f;
		} else {
			r = sp->r;
		}
		AG_DrawCircle(vv, x, y, (int)(r*vv->scale),
		    VG_MapColorRGB(VGNODE(sp)->color));
	}
	if (sp->port != NULL &&
	    sp->port->com != NULL &&
	    sp->port->com->ckt->flags & ES_CIRCUIT_SHOW_NODENAMES) {
		AG_PushTextState();
		AG_TextColor(VG_MapColorRGB(VGNODE(sp)->color));
		Snprintf(text, sizeof(text), "n%d", sp->port->node);
		VG_DrawText(vv,
		    x+10, y+10, 180.0f,
		    text);
		AG_PopTextState();
	}
	if (sp->port == NULL && sp->name[0] != '\0') {
		AG_PushTextState();
		AG_TextFontPct(200);
		AG_TextColor(VG_MapColorRGB(VGNODE(sp)->color));
		VG_DrawText(vv,
		    x+6, y+6, 180.0f,
		    sp->name);
		AG_PopTextState();
	}
}

static void
Extent(void *p, VG_View *vv, VG_Vector *a, VG_Vector *b)
{
	ES_SchemPort *sp = p;
	VG_Vector vPos = VG_Pos(sp);

	a->x = vPos.x - sp->r;
	a->y = vPos.y - sp->r;
	b->x = vPos.x + sp->r;
	b->y = vPos.y + sp->r;
}

static float
PointProximity(void *p, VG_View *vv, VG_Vector *vPt)
{
	ES_SchemPort *sp = p;
	VG_Vector pos = VG_Pos(sp);
	float d;

	d = VG_Distance(pos, *vPt);
	*vPt = pos;
	return (d);
}

static void
Move(void *p, VG_Vector vCurs, VG_Vector vRel)
{
	ES_SchemPort *sp = p;

	VG_Translate(sp, vRel);
}

static void *
Edit(void *p, VG_View *vv)
{
	ES_SchemPort *sp = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);
	AG_Textbox *tb;

#if 0
	AG_LabelNewPolled(box, 0, _("Component: %s(%d)"), &sp->comName,
	    &sp->portName);
	AG_SeparatorNewHoriz(box);
#endif
	tb = AG_TextboxNewS(box, AG_TEXTBOX_HFILL, _("Symbol: "));
	AG_TextboxBindUTF8(tb, sp->name, sizeof(sp->name));

	AG_NumericalNewFlt(box, 0, NULL, _("Display radius: "), &sp->r);

	return (box);
}

VG_NodeOps esSchemPortOps = {
	N_("SchemPort"),
	&esIconPortEditor,
	sizeof(ES_SchemPort),
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
	Edit
};
