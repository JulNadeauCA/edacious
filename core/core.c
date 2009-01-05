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

#include "core.h"
#include <string.h>
#include <ctype.h>

#include <config/moduledir.h>
#include <config/fp_debug.h>

/* #define CLASSDEBUG */

/* Built-in schematic entity (VG) classes. */
void *esSchematicClasses[] = {
	&esSchemBlockOps,
	&esSchemPortOps,
	&esSchemWireOps,
	NULL
};

/* Agar object classes for core Edacious functionality. */
void *esCoreClasses[] = {
	&esCircuitClass,
	&esComponentClass,
	&esWireClass,
	&esScopeClass,
	&esSchemClass,
	&esLayoutClass,
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
int
ES_LoadModule(const char *dsoName)
{
	char sym[AG_DSONAME_MAX+10];
	void *p;
	AG_DSO *dso;
	ES_ComponentClass **comClass;

	AG_LockDSO();

	if ((dso = AG_LoadDSO(dsoName, 0)) == NULL)
		goto fail;

	sym[0] = 'e';
	sym[1] = 's';
	sym[2] = toupper(dsoName[0]);
	Strlcpy(&sym[3], &dsoName[1], sizeof(sym)-3);
	Strlcat(&sym[3], "Module", sizeof(sym)-3);
	if (AG_SymDSO(dso, sym, &p) == -1) {		/* Single component? */
		sym[0] = 'e';
		sym[1] = 's';
		sym[2] = toupper(dsoName[0]);
		Strlcpy(&sym[3], &dsoName[1], sizeof(sym)-3);
		Strlcat(&sym[3], "Class", sizeof(sym)-3);
		if (AG_SymDSO(dso, sym, &p) == -1) {
			goto fail;
		}
		AG_RegisterClass(*(AG_ObjectClass **)p);
#ifdef CLASSDEBUG
		Debug(NULL, "%s: implements %s\n", dsoName,
		    (*(AG_ObjectClass **)p)->name);
#endif
	} else {					/* Complete module */
		ES_Module *mod = p;

		Debug(NULL, "%s: v%s (%s)\n", dsoName, mod->version,
		    mod->descr);
		if (mod->init != NULL) {
			mod->init(EDACIOUS_VERSION);
		}
		for (comClass = mod->comClasses;
		     *comClass != NULL;
		     comClass++) {
			AG_RegisterClass(*comClass);
#ifdef CLASSDEBUG
			Debug(NULL, "%s: implements %s\n", dsoName,
			    ((AG_ObjectClass *)(*comClass))->name);
#endif
		}
	}

	AG_UnlockDSO();
	return (0);
fail:
	AG_UnlockDSO();
	return (-1);
}

/* Unload an Edacious module assuming none of its classes are in use. */
int
ES_UnloadModule(const char *dsoName)
{
	char sym[AG_DSONAME_MAX+10];
	AG_DSO *dso;
	ES_ComponentClass **comClass;
	void *p;

	AG_LockDSO();

	if ((dso = AG_LookupDSO(dsoName)) == NULL)
		goto fail;

	sym[0] = 'e';
	sym[1] = 's';
	sym[2] = toupper(dsoName[0]);
	Strlcpy(&sym[3], &dsoName[1], sizeof(sym)-3);
	Strlcat(&sym[3], "Module", sizeof(sym)-3);
	if (AG_SymDSO(dso, sym, &p) == -1) {		/* Single component? */
		sym[0] = 'e';
		sym[1] = 's';
		sym[2] = toupper(dsoName[0]);
		Strlcpy(&sym[3], &dsoName[1], sizeof(sym)-3);
		Strlcat(&sym[3], "Class", sizeof(sym)-3);
		if (AG_SymDSO(dso, sym, &p) == -1) {
			goto fail;
		}
		AG_UnregisterClass(*(AG_ObjectClass **)p);
	} else {					/* Complete module */
		ES_Module *mod = p;

		Debug(NULL, "%s: v%s (%s)\n", dsoName, mod->version,
		    mod->descr);

		if (mod->destroy != NULL) {
			mod->destroy();
		}
		for (comClass = mod->comClasses; *comClass != NULL; comClass++)
			AG_UnregisterClass(*comClass);
	}

	if (AG_UnloadDSO(dso) == -1)
		goto fail;

	AG_UnlockDSO();
	return (0);
fail:
	AG_UnlockDSO();
	return (-1);
}

/* Initialize the Edacious library. */
void
ES_CoreInit(Uint flags)
{
	void **clsSchem;
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

	/* Register es_core's icons if a GUI is in use. */
	if (agGUI)
		esIcon_Init();

	/*
	 * Register our module directory and preload modules if requested.
	 * Look for an ES_Module definition under "esFooModule".
	 */
	AG_RegisterModuleDirectory(MODULEDIR);
	if (flags & ES_INIT_PRELOAD_ALL &&
	   (dsoList = AG_GetDSOList(&dsoCount)) != NULL) {
		for (i = 0; i < dsoCount; i++) {
			if (ES_LoadModule(dsoList[i]) == -1)
				Debug(NULL, "%s: %s; skipping\n", dsoList[i],
				    AG_GetError());
		}
		AG_FreeDSOList(dsoList, dsoCount);
	}

	ES_ComponentLibraryInit();
	ES_SchemLibraryInit();
}

void
ES_CoreDestroy(void)
{
	AG_DSO *dso, *dsoNext;

	AG_LockDSO();
	ES_ComponentLibraryDestroy();
	ES_SchemLibraryDestroy();

	for (dso = TAILQ_FIRST(&agLoadedDSOs);
	     dso != TAILQ_END(&agLoadedDSOs);
	     dso = dsoNext) {
		dsoNext = TAILQ_NEXT(dso, dsos);
		Debug(NULL, "Unloading: %s\n", dso->name);
		if (ES_UnloadModule(dso->name) == -1)
			Verbose("Unloading %s: %s\n", dso->name, AG_GetError());
	}
	TAILQ_INIT(&agLoadedDSOs);

	AG_UnregisterModuleDirectory(MODULEDIR);
	AG_UnlockDSO();
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

const char *
ES_ShortFilename(const char *name)
{
	char *s;

	if ((s = strrchr(name, PATHSEPCHAR)) != NULL && s[1] != '\0') {
		return (&s[1]);
	} else {
		return (name);
	}
}

void
ES_SetObjectNameFromPath(void *obj, const char *path)
{
	const char *c;

	AG_ObjectSetArchivePath(obj, path);

	if ((c = strrchr(path, PATHSEPCHAR)) != NULL && c[1] != '\0') {
		AG_ObjectSetName(obj, "%s", &c[1]);
	} else {
		AG_ObjectSetName(obj, "%s", path);
	}
}

