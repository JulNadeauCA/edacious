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
 * Port entity. Defines a connection point in the schematic.
 */

#include <eda.h>
#include <agar/gui/primitive.h>
#include <agar/gui/opengl.h>
#include <agar/core/limits.h>

ES_SchemPort *
ES_SchemPortNew(void *pNode)
{
	ES_SchemPort *sp;

	sp = AG_Malloc(sizeof(ES_SchemPort));
	VG_NodeInit(sp, &esSchemPortOps);
	VG_NodeAttach(pNode, sp);
	return (sp);
}

static void
Init(void *p)
{
	ES_SchemPort *sp = p;

	sp->name[0] = '\0';
	sp->r = 3.0f;
	sp->com = NULL;
	sp->port = NULL;
	VG_SetColorRGB(sp, 0, 150, 0);
}

static int
Load(void *p, AG_DataSource *ds, const AG_Version *ver)
{
	ES_SchemPort *sp = p;

	sp->flags = (Uint)AG_ReadUint8(ds);
	AG_CopyString(sp->name, ds, sizeof(sp->name));
	return (0);
}

static void
Save(void *p, AG_DataSource *ds)
{
	ES_SchemPort *sp = p;

	AG_WriteUint8(ds, (Uint8)sp->flags);
	AG_WriteString(ds, sp->name);
}

static void
Draw(void *p, VG_View *vv)
{
	ES_SchemPort *sp = p;
	int x, y;
	float r;

	if (sp->port != NULL && (sp->port->flags & ES_PORT_SELECTED)) {
		r = sp->r+1.0f;
	} else {
		r = sp->r;
	}
	VG_GetViewCoords(vv, VG_Pos(sp), &x, &y);
	AG_DrawCircle(vv, x, y, (int)(r*vv->scale),
	    VG_MapColorRGB(VGNODE(sp)->color));

	if (sp->port != NULL && sp->port->node != -1 &&
	    sp->port->com->ckt->flags & CIRCUIT_SHOW_NODENAMES) {
		char caption[16];
		SDL_Surface *suTmp;
		int su;
	
		AG_PushTextState();
		AG_TextColorVideo32(VG_MapColorRGB(VGNODE(sp)->color));
		Snprintf(caption, sizeof(caption), "n%d", sp->port->node);

		if (agTextCache) {
			su = AG_TextCacheInsLookup(vv->tCache, caption);
		} else {
			suTmp = AG_TextRender(caption);
		}
#ifdef HAVE_OPENGL
		if (agView->opengl) {
			glPushMatrix();
			glTranslatef((float)(AGWIDGET(vv)->cx + x + 10.0f),
			             (float)(AGWIDGET(vv)->cy + y + 10.0f),
				     0.0f);
			glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
			if (agTextCache) {
				AG_WidgetBlitSurfaceGL(vv, su,
				    WSURFACE(vv,su)->w,
				    WSURFACE(vv,su)->h);
			} else {
				AG_WidgetBlitGL(vv, suTmp,
				    suTmp->w,
				    suTmp->h);
				AG_SurfaceFree(suTmp);
			}
			glPopMatrix();
		} else
#endif
		{
			if (agTextCache) {
				AG_WidgetBlitSurface(vv, su, x+4, y+4);
			} else {
				AG_WidgetBlit(vv, suTmp, x+4, y+4);
				AG_SurfaceFree(suTmp);
			}
		}
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

const VG_NodeOps esSchemPortOps = {
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
	Move
};
