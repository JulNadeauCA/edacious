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

#include <config/moduledir.h>

#include "core.h"
#include <string.h>
#include <ctype.h>

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

/* Dummy variable to write everything related to ground to */
M_Real esDummy = 0.0;

#ifdef FP_DEBUG
#include <fenv.h>
#endif

#include "icons_data.h"

AG_Object esVfsRoot;			/* General-purpose VFS */

static AG_Window *(*ObjectOpenFn)(void *) = NULL;
static void       (*ObjectCloseFn)(void *) = NULL;

/* Load an Edacious module and register all of its defined classes. */
static int
LoadModule(const char *dsoName)
{
	char sym[AG_DSONAME_MAX+10], c;
	AG_DSO *dso;
	ES_Module *mod;
	void *p;
	ES_ComponentClass **comClass;
	VG_NodeOps **vgClass;

	if ((dso = AG_LoadDSO(dsoName, 0)) == NULL)
		return (-1);

	/*
	 * Look for an 'esFooModule' description, possibly defining multiple
	 * component and VG schematic classes.
	 */
	sym[0] = 'e';
	sym[1] = 's';
	sym[2] = toupper(dsoName[0]);
	Strlcpy(&sym[3], &dsoName[1], sizeof(sym)-3);
	Strlcat(&sym[3], "Module", sizeof(sym)-3);
	if (AG_SymDSO(dso, sym, &p) == -1) {
		/*
		 * Alternatively, Look for a single 'esFooClass' component
		 * class.
		 */
		sym[0] = 'e';
		sym[1] = 's';
		sym[2] = toupper(dsoName[0]);
		Strlcpy(&sym[3], &dsoName[1], sizeof(sym)-3);
		Strlcat(&sym[3], "Class", sizeof(sym)-3);
		if (AG_SymDSO(dso, sym, &p) == -1) {
			return (-1);
		}
		AG_RegisterClass(*(AG_ObjectClass **)p);
		AG_Verbose("%s.so: implements %s\n", dsoName,
		    (*(AG_ObjectClass **)p)->name);
		return (0);
	}
	
	mod = p;
	AG_Verbose("%s.so: v%s (%s)\n", dsoName, mod->version, mod->descr);
	if (mod->init != NULL) {
		mod->init(EDACIOUS_VERSION);
	}
	for (comClass = mod->comClasses; *comClass != NULL; comClass++) {
		AG_RegisterClass(*comClass);
		AG_Verbose("%s.so: implements %s\n", dsoName,
		    ((AG_ObjectClass *)(*comClass))->name);
	}
	return (0);
}

/* Initialize the Edacious library. */
void
ES_CoreInit(Uint flags)
{
	const void **clsSchem;
	void **cls;
	char **dsoList;
	Uint i, dsoCount;

	VG_InitSubsystem();
	M_InitSubsystem();

	AG_RegisterNamespace("Edacious", "ES_",
	    "http://edacious.hypertriton.com/");

	/* Register the base Agar object and VG entity classes. */
	for (cls = &esCoreClasses[0]; *cls != NULL; cls++)
		AG_RegisterClass(*cls);
	for (clsSchem = &esSchematicClasses[0]; *clsSchem != NULL; clsSchem++)
		VG_RegisterClass(*clsSchem);
	
#ifdef FP_DEBUG
	/* Handle division by zero and overflow. */
	feenableexcept(FE_DIVBYZERO | FE_OVERFLOW);
#endif
	/* Initialize our general-purpose VFS. */
	AG_ObjectInitStatic(&esVfsRoot, NULL);
	AG_ObjectSetName(&esVfsRoot, "Edacious VFS");

	/* Register libcore's icons if a GUI is in use. */
	if (agGUI) {
		esIcon_Init();
	}

	/*
	 * Register our module directory and preload modules if requested.
	 * Look for an ES_Module definition under "esFooModule".
	 */
	AG_RegisterModuleDirectory(MODULEDIR);
	if (flags & ES_INIT_PRELOAD_ALL &&
	   (dsoList = AG_GetDSOList(&dsoCount)) != NULL) {
		for (i = 0; i < dsoCount; i++) {
			if (LoadModule(dsoList[i]) == -1)
				AG_Verbose("%s: %s; skipping\n", dsoList[i],
				    AG_GetError());
		}
		AG_FreeDSOList(dsoList, dsoCount);
	}
}

void
ES_CoreDestroy(void)
{
}

/*
 * Configure a routine allowing the application to open an object for
 * GUI edition. Not thread safe.
 */
void
ES_SetObjectOpenHandler(AG_Window *(*fn)(void *))
{
	ObjectOpenFn = fn;
}

/*
 * Configure a routine allowing the application to clean up GUI resources
 * associated with an object. Not thread safe.
 */
void
ES_SetObjectCloseHandler(void (*fn)(void *))
{
	ObjectCloseFn = fn;
}

AG_Window *
ES_OpenObject(void *obj)
{
	return (ObjectOpenFn != NULL) ? ObjectOpenFn(obj) : NULL;
}

void
ES_CloseObject(void *obj)
{
	if (ObjectCloseFn != NULL)
		ObjectCloseFn(obj);
}
