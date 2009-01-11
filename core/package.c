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
 * Device package. This describes the layout of a device on a PCB.
 * Currently, this is only a dummy subclass of Layout.
 */

#include "core.h"

ES_Package *
ES_PackageNew(ES_Circuit *ckt)
{
	ES_Package *pkg;

	if ((pkg = malloc(sizeof(ES_Package))) == NULL) {
		AG_SetError("Out of memory");
		return (NULL);
	}
	AG_ObjectInit(pkg, &esPackageClass);
	AG_ObjectAttach(ckt, pkg);
	LAYOUT(pkg)->ckt = ckt;
	return (pkg);
}

static void
Init(void *obj)
{
	ES_Package *pkg = obj;

	pkg->flags = 0;
	TAILQ_INIT(&pkg->layoutEnts);
}

static void *
Edit(void *obj)
{
	return AGOBJECT(obj)->cls->super->edit(obj);
}

AG_ObjectClass esPackageClass = {
	"Edacious(Layout:Package)",
	sizeof(ES_Package),
	{ 0,0 },
	Init,
	NULL,		/* reinit */
	NULL,		/* destroy */
	NULL,		/* load */
	NULL,		/* save */
	Edit
};
