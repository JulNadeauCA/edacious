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

#include "core.h"
#include <string.h>

/* Built-in schematic entity (VG) classes. */
const void *esSchematicClasses[] = {
	&esSchemBlockOps,
	&esSchemPortOps,
	&esSchemWireOps,
	NULL
};

/* Agar object classes for core Edacious functionality. */
void *esCoreClasses[] = {
	&esComponentClass,
	&esWireClass,
	&esCircuitClass,
	&esScopeClass,
	&esSchemClass,
	NULL
};

/* User-creatable object classes. */
const char *esEditableClasses[] = {
	"ES_Circuit:*",
	"ES_Schem:*",
	"ES_Scope:*",
	NULL
};

void **esComponentClasses = NULL;		/* Component model classes */
Uint   esComponentClassCount = 0;

#ifdef FP_DEBUG
#include <fenv.h>
#endif

#include "icons_data.h"

AG_Object esVfsRoot;			/* General-purpose VFS */

/* Initialize the Edacious library. */
void
ES_CoreInit(void)
{
	const void **clsSchem;
	void **cls;

	/* Initialize the VG and M libraries. */
	VG_InitSubsystem();
	M_InitSubsystem();

	esComponentClasses = Malloc(sizeof(void *));
	esComponentClassCount = 0;

	/* Register our built-in classes. */
	for (cls = &esCoreClasses[0]; *cls != NULL; cls++)
		AG_RegisterClass(*cls);
	for (clsSchem = &esSchematicClasses[0]; *clsSchem != NULL; clsSchem++)
		VG_RegisterClass(*clsSchem);

#ifdef FP_DEBUG
	/* Handle division by zero and overflow. */
	feenableexcept(FE_DIVBYZERO | FE_OVERFLOW);
#endif
	/* Load our built-in GUI icons. */
	esIcon_Init();
}

void
ES_CoreDestroy(void)
{
}

void
ES_RegisterClass(void *cls)
{
	esComponentClasses = Realloc(esComponentClasses,
	    (esComponentClassCount+1)*sizeof(void *));
	esComponentClasses[esComponentClassCount++] = cls;
	AG_RegisterClass(cls);
}

void
ES_UnregisterClass(void *p)
{
	AG_ObjectClass *cls = p;
	int i;

	for (i = 0; i < esComponentClassCount; i++) {
		if (esComponentClasses[i] == cls)
			break;
	}
	if (i == esComponentClassCount) {
		return;
	}
	if (i < esComponentClassCount-1) {
		memmove(&esComponentClasses[i], &esComponentClasses[i+1],
		    (esComponentClassCount-1)*sizeof(void *));
	}
	esComponentClassCount--;
	AG_UnregisterClass(cls);
}
