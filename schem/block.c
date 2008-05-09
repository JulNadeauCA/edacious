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
 * Schematic block entity. Blocks are rigid and include a pointer to a
 * circuit components. Components may create one or more blocks on attach.
 */

#include <eda.h>
#include <agar/gui/primitive.h>
#include <agar/core/limits.h>

static void
Init(void *p)
{
	ES_SchemBlock *sb = p;

	sb->name[0] = '\0';
}

static int
Load(void *p, AG_DataSource *ds, const AG_Version *ver)
{
	ES_SchemBlock *sb = p;

	AG_CopyString(sb->name, ds, sizeof(sb->name));
	return (0);
}

static void
Save(void *p, AG_DataSource *ds)
{
	ES_SchemBlock *sb = p;

	AG_WriteString(ds, sb->name);
}

static void
GetNodeExtent(VG_Node *vn, VG_View *vv, VG_Rect *r)
{
	VG_Node *vnChld;
	VG_Rect rChld;

	if (vn->ops->extent != NULL) {
		vn->ops->extent(vn, vv, &rChld);
	}
	if (rChld.x < r->x) { r->x = rChld.x; }
	if (rChld.y < r->y) { r->y = rChld.y; }
	if ((rChld.x+rChld.w-r->x) > r->w) {
		r->w = rChld.x+rChld.w-r->x;
	}
	if ((rChld.y+rChld.h-r->y) > r->h) {
		r->h = rChld.y+rChld.h-r->y;
	}
	VG_FOREACH_CHLD(vnChld, vn, vg_node)
		GetNodeExtent(vnChld, vv, r);
}

static void
Extent(void *p, VG_View *vv, VG_Rect *r)
{
	ES_SchemBlock *sb = p;
	VG_Vector vPos = VG_Pos(sb);
	VG_Node *vnChld;

	r->x = vPos.x;
	r->y = vPos.y;
	r->w = 0.0f;
	r->h = 0.0f;

	VG_FOREACH_CHLD(vnChld, sb, vg_node)
		GetNodeExtent(vnChld, vv, r);
}

static void
Draw(void *p, VG_View *vv)
{
#if 0
	ES_SchemBlock *sb = p;
	VG_Rect rExt;
	AG_Rect rDraw;

	Extent(sb, vv, &rExt);
	VG_GetViewCoords(vv, VGVECTOR(rExt.x,rExt.y), &rDraw.x, &rDraw.y);
	rDraw.w = rExt.w*vv->scale;
	rDraw.h = rExt.h*vv->scale;
	AG_DrawRectOutline(vv, rDraw, VG_MapColorRGB(VGNODE(sb)->color));
#endif
}

static float
PointProximity(void *p, VG_View *vv, VG_Vector *vPt)
{
	ES_SchemBlock *sb = p;
	VG_Rect rExt;
	float x = vPt->x;
	float y = vPt->y;
	VG_Vector a, b, c, d;
	float len;

	Extent(sb, vv, &rExt);

	if (x >= rExt.x && x <= rExt.x+rExt.w &&
	    y >= rExt.y && y <= rExt.y+rExt.h) {
		return (0.0f);
	}
	
	a.x = rExt.x;		a.y = rExt.y;
	b.x = rExt.x+rExt.w;	b.y = rExt.y;
	c.x = rExt.x+rExt.w;	c.y = rExt.y+rExt.h;
	d.x = rExt.x;		d.y = rExt.y+rExt.h;

	if (y < a.y) {
		if (x < a.x) {
			len = VG_Distance(a, *vPt);	*vPt = a;
			return (len);
		} else if (x > c.x) { 
			len = VG_Distance(b, *vPt);	*vPt = b;
			return (len);
		} else {
			return VG_PointLineDistance(a,b, vPt);
		}
	} else if (y > c.y) {
		if (x > c.x) {
			len = VG_Distance(c, *vPt);	*vPt = c;
			return (len);
		} else if (x < a.x) {
			len = VG_Distance(d, *vPt);	*vPt = d;
			return (len);
		} else {
			return VG_PointLineDistance(d,c, vPt);
		}
	} else {
		if (x < a.x) {
			return VG_PointLineDistance(a,d, vPt);
		} else if (x > c.x) {
			return VG_PointLineDistance(b,c, vPt);
		}
	}
	return (HUGE_VAL);
}

static void
Move(void *p, VG_Vector vCurs, VG_Vector vRel)
{
	ES_SchemBlock *sb = p;

	VG_Translate(sb, vRel);
}

static void
AttachSubnodesToVG(VG *vg, VG_Node *vn)
{
	VG_Node *vnChld;
	
	VG_FOREACH_CHLD(vnChld, vn, vg_node) {
		AttachSubnodesToVG(vg, vnChld);
	}
	vn->vg = vg;
}

int
ES_SchemBlockLoad(ES_SchemBlock *sb, const char *path)
{
	ES_Schem *scm;
	VG *vgSchem;
	VG *vg = VGNODE(sb)->vg;
	VG_Node *vnRoot;

	scm = ES_SchemNew(NULL);
	if (AG_ObjectLoadFromFile(scm, path) == -1) {
		return (-1);
	}
	vgSchem = scm->vg;
	vnRoot = vgSchem->root;
	vnRoot->vg = vg;
	vnRoot->parent = VGNODE(sb);
	TAILQ_INSERT_TAIL(&vg->nodes, vnRoot, list);
	TAILQ_INSERT_TAIL(&VGNODE(sb)->cNodes, vnRoot, tree);
	AttachSubnodesToVG(vg, vnRoot);
	scm->vg->root = NULL;

	AG_ObjectDestroy(scm);
	return (0);
}

const VG_NodeOps esSchemBlockOps = {
	N_("SchemBlock"),
	&esIconComponent,
	sizeof(ES_SchemBlock),
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
