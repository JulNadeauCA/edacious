/*
 * Copyright (c) 2008-2020 Julien Nadeau Carriere <vedge@csoft.net>
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
	&esPackageClass,
	NULL
};

/* User-creatable object classes. */
const char *esEditableClasses[] = {
	"ES_Circuit:ES_Component:*",
	"ES_Circuit:*",
	"ES_Layout:ES_Package:*",
	"ES_Layout:*",
	"ES_Schem:*",
	NULL
};

/* Dummy variable to write everything related to ground to */
M_Real esDummy = 0.0;

#ifdef FP_DEBUG
#include <fenv.h>
#endif

#include "icons_data.h"

AG_Object esVfsRoot;			/* General-purpose VFS */
static void *objFocus = NULL;		/* For MDI-style interface */
static AG_Menu *mdiMenu = NULL;		/* For MDI-style interface */
static AG_Mutex objLock;
static int appTerminating = 0;

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

	AG_RegisterNamespace("Edacious", "ES_", "http://edacious.org/");

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
	AG_ObjectInit(&esVfsRoot, NULL);
	esVfsRoot.flags |= AG_OBJECT_STATIC;

	AG_ObjectSetName(&esVfsRoot, "Edacious VFS");
	AG_MutexInitRecursive(&objLock);

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

/* Release resources allocated by the Edacious library. */
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

static void
SaveAndClose(AG_Object *obj, AG_Window *win)
{
	AG_ObjectDetach(win);
	AG_ObjectDelete(obj);
}

static void
SaveChangesReturn(AG_Event *event)
{
	AG_Window *win = AG_WINDOW_PTR(1);
	AG_Object *obj = AG_OBJECT_PTR(2);
	const int doSave = AG_INT(3);

	AG_PostEvent(obj, "edit-close", NULL);

	if (doSave) {
		SaveAndClose(obj, win);
	} else {
		AG_ObjectDetach(win);
	}
}

static void
SaveChangesDlg(AG_Event *event)
{
	AG_Window *win = AG_WINDOW_SELF();
	AG_Object *obj = AG_OBJECT_PTR(1);

	if (!AG_ObjectChanged(obj)) {
		SaveAndClose(obj, win);
	} else {
		AG_Button *bOpts[3];
		AG_Window *wDlg;

		wDlg = AG_TextPromptOptions(bOpts, 3,
		    _("Save changes to %s?"), OBJECT(obj)->name);
		AG_WindowAttach(win, wDlg);
		
		AG_ButtonText(bOpts[0], _("Save"));
		AG_SetEvent(bOpts[0], "button-pushed", SaveChangesReturn,
		    "%p,%p,%i", win, obj, 1);
		AG_WidgetFocus(bOpts[0]);
		AG_ButtonText(bOpts[1], _("Discard"));
		AG_SetEvent(bOpts[1], "button-pushed", SaveChangesReturn,
		    "%p,%p,%i", win, obj, 0);
		AG_ButtonText(bOpts[2], _("Cancel"));
		AG_SetEvent(bOpts[2], "button-pushed", AGWINDETACH(wDlg));
	}
}

/* Update the objFocus pointer (only useful in MDI mode). */
static void
WindowGainedFocus(AG_Event *event)
{
	AG_Object *obj = AG_OBJECT_PTR(1);
	const char **s;

	AG_MutexLock(&objLock);
	objFocus = NULL;
	for (s = &esEditableClasses[0]; *s != NULL; s++) {
		if (AG_OfClass(obj, *s)) {
			objFocus = obj;
			break;
		}
	}
	AG_MutexUnlock(&objLock);
}
static void
WindowLostFocus(AG_Event *event)
{
	AG_MutexLock(&objLock);
	objFocus = NULL;
	AG_MutexUnlock(&objLock);
}

/* Open a given object for edition. */
AG_Window *
ES_OpenObject(void *p)
{
	AG_Object *obj = p;
	AG_Window *win = NULL;
	AG_Widget *wEdit;

	/* Invoke edit(), which may return a Window or some other Widget. */
	if (AG_OfClass(obj, "ES_Circuit:ES_Component:*")) {
		win = AGCLASS(&esComponentClass)->edit(obj);
		AG_WindowSetCaption(win, _("Component model: %s"),
		    AG_Defined(obj,"archive-path") ? 
		    AG_ShortFilename(AG_GetStringP(obj,"archive-path")) : obj->name);
	} else {
		if ((wEdit = obj->cls->edit(obj)) == NULL) {
			AG_SetError("%s no edit()", obj->cls->name);
			return (NULL);
		}
		if (AG_OfClass(wEdit, "AG_Widget:AG_Window:*")) {
			win = (AG_Window *)wEdit;
		} else if (AG_OfClass(wEdit, "AG_Widget:*")) {
			win = AG_WindowNew(0);
			AG_ObjectAttach(win, wEdit);
		} else {
			AG_SetError("%s: edit() returned illegal object",
			    obj->cls->name);
			return (NULL);
		}
		AG_WindowSetCaptionS(win,
		    AG_Defined(obj,"archive-path") ?
		    AG_ShortFilename(AG_GetStringP(obj,"archive-path")) : obj->name);
	}

	AG_SetEvent(win, "window-close", SaveChangesDlg, "%p", obj);
	AG_AddEvent(win, "window-gainfocus", WindowGainedFocus, "%p", obj);
	AG_AddEvent(win, "window-lostfocus", WindowLostFocus, "%p", obj);
	AG_AddEvent(win, "window-hidden", WindowLostFocus, "%p", obj);
	AG_SetPointer(win, "object", obj);
	AG_PostEvent(obj, "edit-open", NULL);

	AG_WindowShow(win);
	return (win);
}

/*
 * Close any edition window associated with a given Edacious object
 * (window "object" property).
 */
void
ES_CloseObject(void *obj)
{
	AG_Driver *drv;
	AG_Window *win;
	AG_Variable *V;

	/* XXX XXX agar-1.4 */
	AGOBJECT_FOREACH_CHILD(drv, &agDrivers, ag_driver) {
		AG_FOREACH_WINDOW(win, drv) {
			if ((V = AG_AccessVariable(win, "object")) == NULL) {
				continue;
			}
			if (V->data.p == obj) {
				AG_UnlockVariable(V);
				AG_ObjectDetach(win);
			} else {
				AG_UnlockVariable(V);
			}
		}
	}
}

/* Create a new instance of a particular object class. */
void
ES_GUI_NewObject(AG_Event *event)
{
	AG_ObjectClass *cl = AG_PTR(1);
	AG_Object *obj;

	if ((obj = AG_ObjectNew(&esVfsRoot, NULL, cl)) == NULL) {
		goto fail;
	}
	if (ES_OpenObject(obj) == NULL) {
		goto fail;
	}
	return;
fail:
	AG_TextError(_("Failed to create object: %s"), AG_GetError());
	if (obj != NULL) { AG_ObjectDelete(obj); }
}

/* Create a new component model. */
void
ES_GUI_CreateComponentModel(AG_Event *event)
{
	AG_Window *winParent = AG_WINDOW_PTR(1);
	AG_Tlist *tlClasses = AG_TLIST_PTR(2);
	AG_TlistItem *itClass;
	ES_Component *com;

	if ((itClass = AG_TlistSelectedItem(tlClasses)) == NULL) {
		AG_TextError(_("Please select a parent class"));
		return;
	}
	if ((com = AG_ObjectNew(&esVfsRoot, NULL, AGCLASS(itClass->p1)))
	    == NULL) {
		goto fail;
	}
	if (ES_OpenObject(com) == NULL) {
		goto fail;
	}
	AG_ObjectDetach(winParent);
	return;
fail:
	AG_TextError(_("Could not create component: %s"), AG_GetError());
	if (com != NULL) { AG_ObjectDelete(com); }
}

/* Display the "File / New component" dialog. */
void
ES_GUI_NewComponentModelDlg(AG_Event *event)
{
	AG_Window *win;
	AG_Tlist *tlHier;
	AG_Box *hBox;

	win = AG_WindowNew(0);
	AG_WindowSetCaptionS(win, _("New component..."));

	AG_LabelNewS(win, AG_LABEL_HFILL,
	    _("Please select a class for the new component model."));
	AG_SeparatorNewHoriz(win);

	AG_LabelNew(win, 0, _("Class: "));
	tlHier = AG_TlistNewPolled(win, AG_TLIST_TREE | AG_TLIST_EXPAND,
	                           ES_ComponentListClasses, NULL);

	hBox = AG_BoxNewHoriz(win, AG_BOX_HOMOGENOUS | AG_BOX_FRAME |
	                           AG_BOX_HFILL);

	AG_ButtonNewFn(hBox, 0, _("Create model"),
	    ES_GUI_CreateComponentModel, "%p,%p", win, tlHier);

	AG_ButtonNewFn(hBox, 0, _("Close"), AGWINDETACH(win));

	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 33, 40);
	AG_WindowShow(win);
}

#if 0
static void
EditDevice(AG_Event *event)
{
	AG_Object *dev = AG_OBJECT_PTR(1);

	ES_OpenObject(dev);
}
#endif

/* Load an object file from native Edacious format. */
void
ES_GUI_LoadObject(AG_Event *event)
{
	AG_ObjectClass *cl = AG_PTR(1);
	const char *path = AG_STRING(2);
	AG_Object *obj;

	if ((obj = AG_ObjectNew(&esVfsRoot, NULL, cl)) == NULL) {
		AG_TextMsgFromError();
		return;
	}
	if (AG_ObjectLoadFromFile(obj, path) == -1) {
		AG_SetError("%s: %s", AG_ShortFilename(path), AG_GetError());
		goto fail;
	}
	AG_SetString(obj, "archive-path", path);
	AG_ObjectSetNameS(obj, AG_ShortFilename(path));
	if (ES_OpenObject(obj) == NULL) {
		goto fail;
	}
	return;
fail:
	AG_TextMsgFromError();
	AG_ObjectDestroy(obj);
}

/* Load a component model file. */
static void
LoadComponentModel(AG_Event *event)
{
	const char *path = AG_STRING(1);
	AG_ObjectHeader oh;
	AG_DataSource *ds;
	AG_Object *obj;
	AG_ObjectClass *cl;

	if ((ds = AG_OpenFile(path, "rb")) == NULL) {
		goto fail;
	}
	if (AG_ObjectReadHeader(ds, &oh) == -1) {
		AG_CloseFile(ds);
		goto fail;
	}
	AG_CloseFile(ds);

	if ((cl = AG_LoadClass(oh.cs.hier)) == NULL) {
		goto fail;
	}
	if ((obj = AG_ObjectNew(&esVfsRoot, NULL, cl)) == NULL) {
		goto fail;
	}
	if (AG_ObjectLoadFromFile(obj, path) == -1) {
		AG_SetError("%s: %s", AG_ShortFilename(path), AG_GetError());
		goto fail_obj;
	}
	AG_SetString(obj, "archive-path", path);
	AG_ObjectSetNameS(obj, AG_ShortFilename(path));

	if (ES_OpenObject(obj) == NULL) {
		goto fail_obj;
	}
	return;
fail_obj:
	AG_ObjectDestroy(obj);
fail:
	AG_TextMsgFromError();
}

void
ES_GUI_OpenDlg(AG_Event *event)
{
	AG_Window *win;
	AG_FileDlg *fd;

	win = AG_WindowNew(0);
	AG_WindowSetCaptionS(win, _("Open..."));

	fd = AG_FileDlgNewMRU(win, "edacious.mru.circuits",
	    AG_FILEDLG_LOAD | AG_FILEDLG_CLOSEWIN | AG_FILEDLG_EXPAND);

	AG_FileDlgSetOptionContainer(fd, AG_BoxNewVert(win, AG_BOX_HFILL));

	AG_FileDlgAddType(fd, _("Edacious circuit model"), "*.ecm",
	    ES_GUI_LoadObject, "%p", &esCircuitClass);
	AG_FileDlgAddType(fd, _("Edacious component model"), "*.em",
	    LoadComponentModel, NULL);
	AG_FileDlgAddType(fd, _("Edacious schematic"), "*.esh",
	    ES_GUI_LoadObject, "%p", &esSchemClass);
	AG_FileDlgAddType(fd, _("Edacious PCB layout"), "*.ecl",
	    ES_GUI_LoadObject, "%p", &esLayoutClass);
	AG_FileDlgAddType(fd, _("Edacious device package"), "*.edp",
	    ES_GUI_LoadObject, "%p", &esPackageClass);

	AG_WindowShow(win);
}

/* Save an object file in native Edacious format. */
static void
SaveNativeObject(AG_Event *event)
{
	AG_Object *obj = AG_OBJECT_PTR(1);
	const char *path = AG_STRING(2);
	AG_Window *wEdit;

	if (AG_ObjectSaveToFile(obj, path) == -1) {
		AG_TextMsgFromError();
		return;
	}
	AG_SetString(obj, "archive-path", path);
	AG_ObjectSetNameS(obj, AG_ShortFilename(path));

	if ((wEdit = AG_WindowFindFocused()) != NULL)
		AG_WindowSetCaptionS(wEdit, AG_ShortFilename(path));
}

static void
SaveCircuitToSPICE3(AG_Event *event)
{
	ES_Circuit *ckt = ES_CIRCUIT_PTR(1);
	const char *path = AG_STRING(2);
	
	if (ES_CircuitExportSPICE3(ckt, path) == -1) {
		AG_TextMsgFromError();
		return;
	}
	AG_TextTmsg(AG_MSG_INFO, 1250,
	    _("Exported %s successfully to %s"),
	    OBJECT(ckt)->name, AG_ShortFilename(path));
}

static void
SaveCircuitToTXT(AG_Event *event)
{
	ES_Circuit *ckt = ES_CIRCUIT_PTR(1);
	const char *path = AG_STRING(2);

	if (ES_CircuitExportTXT(ckt, path) == -1) {
		AG_TextMsgFromError();
		return;
	}
	AG_TextTmsg(AG_MSG_INFO, 1250,
	    _("Exported %s successfully to %s"),
	    OBJECT(ckt)->name, AG_ShortFilename(path));
	
}

void
ES_GUI_SaveAsDlg(AG_Event *event)
{
	char defDir[AG_PATHNAME_MAX];
	AG_Object *obj = AG_OBJECT_PTR(1);
	AG_Window *win;
	AG_FileDlg *fd;

	if (obj == NULL) {
		AG_TextError(_("No document is selected for saving."));
		return;
	}

	AG_GetString(agConfig, "save-path", defDir, sizeof(defDir));

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Save %s as..."), obj->name);

	fd = AG_FileDlgNew(win, AG_FILEDLG_SAVE|AG_FILEDLG_CLOSEWIN|
	                        AG_FILEDLG_EXPAND);
	AG_FileDlgSetOptionContainer(fd, AG_BoxNewVert(win, AG_BOX_HFILL));

	if (AG_OfClass(obj, "ES_Circuit:ES_Component:*")) {
		AG_FileDlgSetDirectoryMRU(fd, "edacious.mru.components", defDir);
		AG_FileDlgAddType(fd, _("Edacious component model"),
		    "*.em",
		    SaveNativeObject, "%p", obj);
	} else if (AG_OfClass(obj, "ES_Circuit:*")) {
		AG_FileDlgSetDirectoryMRU(fd, "edacious.mru.circuits", defDir);
		AG_FileDlgAddType(fd, _("Edacious circuit model"),
		    "*.ecm",
		    SaveNativeObject, "%p", obj);
		AG_FileDlgAddType(fd, _("SPICE3 netlist"),
		    "*.cir",
		    SaveCircuitToSPICE3, "%p", obj);
		AG_FileDlgAddType(fd, _("Text file"),
		    "*.txt",
		    SaveCircuitToTXT, "%p", obj);
		/* ... */
	} else if (AG_OfClass(obj, "ES_Layout:ES_Package:*")) {
		AG_FileDlgSetDirectoryMRU(fd, "edacious.mru.packages", defDir);
		AG_FileDlgAddType(fd, _("Edacious device package"),
		    "*.edp",
		    SaveNativeObject, "%p", obj);
	} else if (AG_OfClass(obj, "ES_Layout:*")) {
		AG_FileDlgSetDirectoryMRU(fd, "edacious.mru.layouts", defDir);
		AG_FileDlgAddType(fd, _("Edacious PCB layout"),
		    "*.ecl",
		    SaveNativeObject, "%p", obj);
#if 0
		AG_FileDlgAddType(fd, _("gEDA PCB format"),
		    "*.pcb",
		    SaveLayoutToGedaPCB, "%p", obj);
		AG_FileDlgAddType(fd, _("Gerber (RS-274D)"),
		    "*.gbr,*.phd,*.spl,*.art",
		    SaveLayoutToGerber, "%p", obj);
		AG_FileDlgAddType(fd, _("Extended Gerber (RS-274X)"),
		    "*.ger,*.gbl,*.gtl,*.gbs,*.gts,*.gbo,*.gto",
		    SaveLayoutToXGerber, "%p", obj);
#endif
	} else if (AG_OfClass(obj, "ES_Schem:*")) {
		AG_FileDlgSetDirectoryMRU(fd, "edacious.mru.schems", defDir);
		AG_FileDlgAddType(fd, _("Edacious schematic"),
		    "*.esh",
		    SaveNativeObject, "%p", obj);
#if 0
		AG_FileDlgAddType(fd, _("Portable Document Format"),
		    "*.pdf",
		    SaveSchemToPDF, "%p", obj);
#endif
	}
	AG_WindowShow(win);
}

void
ES_GUI_Save(AG_Event *event)
{
	AG_Object *obj = AG_OBJECT_PTR(1);
	
	if (obj == NULL) {
		AG_TextError(_("No document is selected for saving."));
		return;
	}

	if (!AG_Defined(obj,"archive-path")) {
		ES_GUI_SaveAsDlg(event);
		return;
	}
	if (AG_ObjectSave(obj) == -1) {
		AG_TextError(_("Error saving object: %s"), AG_GetError());
	} else {
		AG_TextTmsg(AG_MSG_INFO, 1250, _("Saved %s successfully"),
		    AG_GetStringP(obj,"archive-path"));
	}
}

static void
ConfirmQuit(AG_Event *event)
{
	AG_QuitGUI();
}

static void
AbortQuit(AG_Event *event)
{
	AG_Window *win = AG_WINDOW_PTR(1);

	appTerminating = 0;
	AG_ObjectDetach(win);
}

void
ES_GUI_Quit(AG_Event *event)
{
	AG_Object *vfsObj = NULL, *modelObj = NULL, *schemObj = NULL;
	AG_Window *win;
	AG_Checkbox *cb;
	AG_Box *box;

	if (appTerminating) {
		ConfirmQuit(NULL);
	}
	appTerminating = 1;

	OBJECT_FOREACH_CHILD(vfsObj, &esVfsRoot, ag_object) {
		if (AG_ObjectChanged(vfsObj))
			break;
	}
	OBJECT_FOREACH_CHILD(modelObj, &esComponentLibrary, ag_object) {
		if (AG_ObjectChanged(modelObj))
			break;
	}
	OBJECT_FOREACH_CHILD(schemObj, &esSchemLibrary, ag_object) {
		if (AG_ObjectChanged(schemObj))
			break;
	}

	if (vfsObj == NULL &&
	    modelObj == NULL &&
	    schemObj == NULL) {
		if (!AG_GetInt(agConfig,"no-confirm-quit")) {
			ConfirmQuit(NULL);
		} else {
			AG_Variable *Vdisable;

			if ((win = AG_WindowNewNamedS(
			    AG_WINDOW_MODAL|AG_WINDOW_NOTITLE|
			    AG_WINDOW_NORESIZE, "QuitCallback")) == NULL) {
				return;
			}
			AG_WindowSetCaptionS(win, _("Quit application?"));
			AG_WindowSetPosition(win, AG_WINDOW_CENTER, 0);
			AG_WindowSetSpacing(win, 8);

			AG_LabelNewS(win, 0, _("Exit Edacious?"));
			cb = AG_CheckboxNew(win, 0, _("Don't ask again"));
			Vdisable = AG_SetInt(agConfig,"no-confirm-quit",0);
			AG_BindInt(cb, "state", &Vdisable->data.i);

			box = AG_BoxNewHorizNS(win, AG_VBOX_HFILL);
			AG_ButtonNewFn(box, 0, _("Quit"), ConfirmQuit, NULL);
			AG_WidgetFocus(AG_ButtonNewFn(box, 0, _("Cancel"), AbortQuit, "%p", win));
			AG_WindowShow(win);
		}
	} else {
		if ((win = AG_WindowNewNamedS(AG_WINDOW_MODAL|AG_WINDOW_NOTITLE|
		    AG_WINDOW_NORESIZE, "QuitCallback")) == NULL) {
			return;
		}
		AG_WindowSetCaptionS(win, _("Exit application?"));
		AG_WindowSetPosition(win, AG_WINDOW_CENTER, 0);
		AG_WindowSetSpacing(win, 8);
		AG_LabelNewS(win, 0,
		    _("There is at least one object with unsaved changes.  "
	              "Exit application?"));
		box = AG_BoxNewHorizNS(win, AG_VBOX_HFILL|AG_BOX_HOMOGENOUS);
		AG_ButtonNewFn(box, 0, _("Discard changes"), ConfirmQuit, NULL);
		AG_WidgetFocus(AG_ButtonNewFn(box, 0, _("Cancel"), AbortQuit, "%p", win));
		AG_WindowShow(win);
	}
}

#if 0
static void
VideoResize(Uint w, Uint h)
{
	AG_SetUint(agConfig,"gui-width", w);
	AG_SetUint(agConfig,"gui-height", h);
	(void)AG_ConfigSave();
}
#endif

void
ES_GUI_Undo(AG_Event *event)
{
	/* TODO */
	printf("undo!\n");
}

void
ES_GUI_Redo(AG_Event *event)
{
	/* TODO */
	printf("redo!\n");
}

void
ES_GUI_EditPreferences(AG_Event *event)
{
	DEV_ConfigShow();
}

static void
SelectedFont(AG_Event *event)
{
	AG_Window *win = AG_WINDOW_PTR(1);

	AG_SetString(agConfig, "font.face", OBJECT(agDefaultFont)->name);
	AG_SetInt(agConfig, "font.size", agDefaultFont->spec.size);
	AG_SetUint(agConfig, "font.flags", agDefaultFont->flags);
	(void)AG_ConfigSave();

	AG_TextWarning("default-font-changed",
	    _("The default font has been changed.\n"
	      "Please restart Edacious for this change to take effect."));
	AG_ObjectDetach(win);
}

void
ES_GUI_SelectFontDlg(AG_Event *event)
{
	AG_Window *win;
	AG_FontSelector *fs;
	AG_Box *hBox;

	win = AG_WindowNew(0);
	AG_WindowSetCaptionS(win, _("Font selection"));

	fs = AG_FontSelectorNew(win, AG_FONTSELECTOR_EXPAND);
	AG_BindPointer(fs, "font", (void *)&agDefaultFont);

	hBox = AG_BoxNewHoriz(win, AG_BOX_HFILL|AG_BOX_HOMOGENOUS);
	AG_ButtonNewFn(hBox, 0, _("OK"), SelectedFont, "%p", win);
	AG_ButtonNewFn(hBox, 0, _("Cancel"), AG_WindowCloseGenEv, "%p", win);
	AG_WindowShow(win);
}

/* Initialize MDI-style menu. */
void
ES_InitMenuMDI(void)
{
	if ((mdiMenu = AG_MenuNewGlobal(0)) == NULL) {
		AG_Verbose("App menu: %s\n", AG_GetError());
		return;
	}
	ES_FileMenu(AG_MenuNode(mdiMenu->root, _("File"), NULL), NULL);
	ES_EditMenu(AG_MenuNode(mdiMenu->root, _("Edit"), NULL), NULL);
#if defined(HAVE_AGAR_DEV) && defined(ES_DEBUG)
	DEV_InitSubsystem(0);
	DEV_ToolMenu(AG_MenuNode(mdiMenu->root, _("Debug")));
#endif
}

/* Build a generic "File" menu. */
void
ES_FileMenu(AG_MenuItem *m, void *obj)
{
	if (obj == NULL)					/* MDI */
		obj = objFocus;

	AG_MenuActionKb(m, _("New circuit..."), esIconCircuit.s,
	    AG_KEY_N, AG_KEYMOD_CTRL,
	    ES_GUI_NewObject, "%p", &esCircuitClass);
	AG_MenuAction(m, _("New component model..."), esIconComponent.s,
	    ES_GUI_NewComponentModelDlg, NULL);
	AG_MenuAction(m, _("New schematic..."), vgIconDrawing.s,
	    ES_GUI_NewObject, "%p", &esSchemClass);
	AG_MenuAction(m, _("New PCB layout..."), vgIconDrawing.s,
	    ES_GUI_NewObject, "%p", &esLayoutClass);
	AG_MenuAction(m, _("New device package..."), vgIconDrawing.s,
	    ES_GUI_NewObject, "%p", &esPackageClass);
	
	AG_MenuSeparator(m);

	AG_MenuActionKb(m, _("Open..."), agIconLoad.s,
	    AG_KEY_O, AG_KEYMOD_CTRL,
	    ES_GUI_OpenDlg, NULL);
	AG_MenuActionKb(m, _("Save"), agIconSave.s,
	    AG_KEY_S, AG_KEYMOD_CTRL,
	    ES_GUI_Save, "%p", obj);
	AG_MenuAction(m, _("Save as..."), agIconSave.s,
	    ES_GUI_SaveAsDlg, "%p", obj);

#if 0
	node = AG_MenuNode(m, _("Devices"), NULL);
	{
		ES_Device *dev;
		AG_MenuAction(node, _("Microcontroller..."), agIconDoc.s,
		    ES_GUI_NewObject, "%p", &esMCU);
		AG_MenuSeparator(node);
		OBJECT_FOREACH_CLASS(dev, &esVfsRoot, es_device,
		    "ES_Device:*") {
			AG_MenuAction(node, OBJECT(dev)->name, agIconGear.s,
			    EditDevice, "%p", dev);
		}
	}
#endif

	AG_MenuSeparator(m);
	AG_MenuActionKb(m, _("Quit"), agIconClose.s,
	    AG_KEY_Q, AG_KEYMOD_CTRL,
	    ES_GUI_Quit, NULL);
}

/* Build a generic "Edit" menu. */
void
ES_EditMenu(AG_MenuItem *m, void *obj)
{
	if (obj == NULL)					/* MDI */
		obj = objFocus;

	AG_MenuActionKb(m, _("Undo"), agIconUp.s, AG_KEY_Z, AG_KEYMOD_CTRL,
	    ES_GUI_Undo, "%p", obj);
	AG_MenuActionKb(m, _("Redo"), agIconDown.s, AG_KEY_R, AG_KEYMOD_CTRL,
	    ES_GUI_Redo, "%p", obj);
	
	AG_MenuSeparator(m);

	AG_MenuAction(m, _("Select font..."), vgIconText.s,
	    ES_GUI_SelectFontDlg, NULL);
	AG_MenuAction(m, _("Preferences..."), agIconGear.s,
	    ES_GUI_EditPreferences, NULL);
}
