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
 * Component schematic. This is mainly a vector drawing with specific
 * functionality for ports and labels.
 */

#include "core.h"

ES_Layout *
ES_LayoutNew(ES_Circuit *ckt)
{
	ES_Layout *lo;

	if ((lo = malloc(sizeof(ES_Layout))) == NULL) {
		AG_SetError("Out of memory");
		return (NULL);
	}
	AG_ObjectInit(lo, &esLayoutClass);
	AG_ObjectAttach(ckt, lo);
	lo->ckt = ckt;
	return (lo);
}

static void
Init(void *obj)
{
	ES_Layout *lo = obj;

	lo->vg = VG_New(0);
	lo->ckt = NULL;
	TAILQ_INIT(&lo->packages);
}

static void
Reinit(void *obj)
{
	ES_Layout *lo = obj;

	VG_Clear(lo->vg);
}

static void
Destroy(void *obj)
{
	ES_Layout *lo = obj;

	VG_Destroy(lo->vg);
	free(lo->vg);
}

static int
Load(void *obj, AG_DataSource *ds, const AG_Version *ver)
{
	ES_Layout *lo = obj;
	
	if (VG_Load(lo->vg, ds) == -1)
		return (-1);
	
	return (0);
}

static int
Save(void *obj, AG_DataSource *ds)
{
	ES_Layout *lo = obj;

	VG_Save(lo->vg, ds);
	return (0);
}

AG_ObjectClass esLayoutClass = {
	"Edacious(Layout)",
	sizeof(ES_Layout),
	{ 0,0 },
	Init,
	Reinit,
	Destroy,
	Load,
	Save,
	ES_LayoutEdit
};
