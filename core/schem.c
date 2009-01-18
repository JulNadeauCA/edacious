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
 * General-purpose schematic drawing.
 */

#include "core.h"

ES_Schem *
ES_SchemNew(void *parent)
{
	ES_Schem *scm;

	scm = AG_Malloc(sizeof(ES_Schem));
	AG_ObjectInit(scm, &esSchemClass);
	AG_ObjectAttach(parent, scm);
	return (scm);
}

static void
Init(void *obj)
{
	ES_Schem *scm = obj;

	scm->vg = VG_New(0);
}

static void
Reinit(void *obj)
{
	ES_Schem *scm = obj;

	VG_Clear(scm->vg);
}

static void
Destroy(void *obj)
{
	ES_Schem *scm = obj;

	VG_Destroy(scm->vg);
	free(scm->vg);
}

static int
Load(void *obj, AG_DataSource *ds, const AG_Version *ver)
{
	ES_Schem *scm = obj;
	
	return VG_Load(scm->vg, ds);
}

static int
Save(void *obj, AG_DataSource *ds)
{
	ES_Schem *scm = obj;

	VG_Save(scm->vg, ds);
	return (0);
}

/* Return the entity nearest to vPos. */
void *
ES_SchemNearest(VG_View *vv, VG_Vector vPos)
{
	VG *vg = vv->vg;
	float prox, proxNearest;
	VG_Node *vn, *vnNearest;
	VG_Vector v;

	/* First check if we intersect a block. */
	TAILQ_FOREACH(vn, &vg->nodes, list) {
		if (!VG_NodeIsClass(vn, "SchemBlock")) {
			continue;
		}
		v = vPos;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox == 0.0f)
			return (vn);
	}

	/* Then prioritize points at a fixed distance. */
	proxNearest = AG_FLT_MAX;
	vnNearest = NULL;
	TAILQ_FOREACH(vn, &vg->nodes, list) {
		if (!VG_NodeIsClass(vn, "Point")) {
			continue;
		}
		v = vPos;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox <= vv->pointSelRadius) {
			if (prox < proxNearest) {
				proxNearest = prox;
				vnNearest = vn;
			}
		}
	}
	if (vnNearest != NULL)
		return (vnNearest);

	/* Finally, fallback to a general query. */
	proxNearest = AG_FLT_MAX;
	vnNearest = NULL;
	TAILQ_FOREACH(vn, &vg->nodes, list) {
		if (vn->ops->pointProximity == NULL) {
			continue;
		}
		v = vPos;
		prox = vn->ops->pointProximity(vn, vv, &v);
		if (prox < proxNearest) {
			proxNearest = prox;
			vnNearest = vn;
		}
	}
	return (vnNearest);
}

AG_ObjectClass esSchemClass = {
	"Edacious(Schem)",
	sizeof(ES_Schem),
	{ 0,0 },
	Init,
	Reinit,
	Destroy,
	Load,
	Save,
	ES_SchemEdit
};
