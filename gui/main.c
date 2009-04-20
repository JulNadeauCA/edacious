/*
 * Copyright (c) 2003-2009 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Main user interface code for Edacious.
 */

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/dev.h>
#include <agar/vg.h>

#include <core/core.h>
#include <generic/generic.h>
#include <macro/macro.h>
#include <sources/sources.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <config/enable_nls.h>
#include <config/have_getopt.h>
#include <config/have_agar_dev.h>
#include <agar/config/have_opengl.h>

AG_Menu *appMenu = NULL;
static void *objFocus = NULL;
static AG_Mutex objLock;

static void
SaveAndClose(AG_Object *obj, AG_Window *win)
{
	AG_ViewDetach(win);
	AG_ObjectDelete(obj);
}

static void
SaveChangesReturn(AG_Event *event)
{
	AG_Window *win = AG_PTR(1);
	AG_Object *obj = AG_PTR(2);
	int doSave = AG_INT(3);

	AG_PostEvent(NULL, obj, "edit-close", NULL);

	if (doSave) {
		SaveAndClose(obj, win);
	} else {
		AG_ViewDetach(win);
	}
}

static void
SaveChangesDlg(AG_Event *event)
{
	AG_Window *win = AG_SELF();
	AG_Object *obj = AG_PTR(1);

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

static void
WindowGainedFocus(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);
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
static AG_Window *
OpenObject(void *p)
{
	AG_Object *obj = p;
	AG_Window *win = NULL;
	AG_Widget *wEdit;

	/* Invoke edit(), which may return a Window or some other Widget. */
	if (AG_OfClass(obj, "ES_Circuit:ES_Component:*")) {
		win = AGCLASS(&esComponentClass)->edit(obj);
		AG_WindowSetCaption(win, _("Component model: %s"),
		    (obj->archivePath != NULL) ?
		    ES_ShortFilename(obj->archivePath) : obj->name);
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
		AG_WindowSetCaption(win, "%s",
		    (obj->archivePath != NULL) ?
		    ES_ShortFilename(obj->archivePath) : obj->name);
	}

	AG_SetEvent(win, "window-close", SaveChangesDlg, "%p", obj);
	AG_AddEvent(win, "window-gainfocus", WindowGainedFocus, "%p", obj);
	AG_AddEvent(win, "window-lostfocus", WindowLostFocus, "%p", obj);
	AG_AddEvent(win, "window-hidden", WindowLostFocus, "%p", obj);
	AG_SetPointer(win, "object", obj);

	AG_WindowShow(win);
	return (win);
}

/* Close edition window associated with an object. */
static void
CloseObject(void *obj)
{
	AG_Window *win;
	AG_Variable *V;

	TAILQ_FOREACH(win, &agView->windows, windows) {
		if ((V = AG_GetVariableLocked(win, "object")) != NULL) {
			if (V->data.p == obj) {
				AG_ViewDetach(win);
			}
			AG_UnlockVariable(V);
		}
	}
}

/* Create a new instance of a particular object class. */
static void
NewObject(AG_Event *event)
{
	AG_ObjectClass *cl = AG_PTR(1);
	AG_Object *obj;

	if ((obj = AG_ObjectNew(&esVfsRoot, NULL, cl)) == NULL) {
		goto fail;
	}
	if (OpenObject(obj) == NULL) {
		goto fail;
	}
	AG_PostEvent(NULL, obj, "edit-open", NULL);
	return;
fail:
	AG_TextError(_("Failed to create object: %s"), AG_GetError());
	if (obj != NULL) { AG_ObjectDelete(obj); }
}

/* Create a new component model. */
static void
CreateComponentModel(AG_Event *event)
{
	AG_Tlist *tlClasses = AG_PTR(1);
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
	if (OpenObject(com) == NULL) {
		goto fail;
	}
	AG_PostEvent(NULL, com, "edit-open", NULL);
	return;
fail:
	AG_TextError(_("Could not create component: %s"), AG_GetError());
	if (com != NULL) { AG_ObjectDelete(com); }
}

/* Display the "File / New component" dialog. */
static void
NewComponentModelDlg(AG_Event *event)
{
	AG_Window *win;
	AG_Tlist *tlHier;
	AG_Box *hBox;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("New component..."));
	
	AG_LabelNew(win, 0, _("Class: "));
	tlHier = AG_TlistNewPolled(win, AG_TLIST_TREE|AG_TLIST_EXPAND,
	    ES_ComponentListClasses, NULL);

	hBox = AG_BoxNewHoriz(win, AG_BOX_HOMOGENOUS|AG_BOX_FRAME|AG_BOX_HFILL);
	AG_ButtonNewFn(hBox, 0, _("Create model"),
	    CreateComponentModel, "%p", tlHier);
	AG_ButtonNewFn(hBox, 0, _("Close"), AGWINDETACH(win));

	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 60, 60);
	AG_WindowShow(win);
}

#if 0
static void
EditDevice(AG_Event *event)
{
	AG_Object *dev = AG_PTR(1);

	OpenObject(dev);
}
#endif

/* Load an object file from native Edacious format. */
static void
LoadObject(AG_Event *event)
{
	AG_ObjectClass *cl = AG_PTR(1);
	char *path = AG_STRING(2);
	AG_Object *obj;

	if ((obj = AG_ObjectNew(&esVfsRoot, NULL, cl)) == NULL) {
		goto fail;
	}
	if (AG_ObjectLoadFromFile(obj, path) == -1) {
		AG_SetError("%s: %s", ES_ShortFilename(path), AG_GetError());
		goto fail;
	}
	ES_SetObjectNameFromPath(obj, path);

	if (OpenObject(obj) == NULL) {
		goto fail;
	}
	return;
fail:
	AG_TextError(_("Could not open object: %s"), AG_GetError());
/*	if (obj != NULL) { AG_ObjectDelete(obj); } */
}

/* Load a component model file. */
static void
LoadComponentModel(AG_Event *event)
{
	AG_ObjectHeader oh;
	AG_DataSource *ds;
	char *path = AG_STRING(1);
	AG_Object *obj;
	AG_ObjectClass *cl;

	if ((ds = AG_OpenFile(path, "rb")) == NULL) {
		AG_TextMsgFromError();
		return;
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
		AG_SetError("%s: %s", ES_ShortFilename(path), AG_GetError());
		goto fail;
	}
	ES_SetObjectNameFromPath(obj, path);

	if (OpenObject(obj) == NULL) {
		goto fail;
	}
	return;
fail:
/*	if (obj != NULL) { AG_ObjectDelete(obj); } */
	AG_TextMsgFromError();
}

static void
OpenDlg(AG_Event *event)
{
	AG_Window *win;
	AG_FileDlg *fd;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Open..."));

	fd = AG_FileDlgNewMRU(win, "edacious.mru.circuits",
	    AG_FILEDLG_LOAD|AG_FILEDLG_CLOSEWIN|AG_FILEDLG_EXPAND);
	AG_FileDlgSetOptionContainer(fd, AG_BoxNewVert(win, AG_BOX_HFILL));

	AG_FileDlgAddType(fd, _("Edacious circuit model"), "*.ecm",
	    LoadObject, "%p", &esCircuitClass);
	AG_FileDlgAddType(fd, _("Edacious component model"), "*.em",
	    LoadComponentModel, NULL);
	AG_FileDlgAddType(fd, _("Edacious schematic"), "*.esh",
	    LoadObject, "%p", &esSchemClass);
	AG_FileDlgAddType(fd, _("Edacious PCB layout"), "*.ecl",
	    LoadObject, "%p", &esLayoutClass);
	AG_FileDlgAddType(fd, _("Edacious device package"), "*.edp",
	    LoadObject, "%p", &esPackageClass);

	AG_WindowShow(win);
}

/* Save an object file in native Edacious format. */
static void
SaveNativeObject(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);
	char *path = AG_STRING(2);
	AG_Window *wEdit;

	if (AG_ObjectSaveToFile(obj, path) == -1) {
		AG_TextError("%s: %s", ES_ShortFilename(path), AG_GetError());
	}
	ES_SetObjectNameFromPath(obj, path);

	if ((wEdit = AG_WindowFindFocused()) != NULL)
		AG_WindowSetCaption(wEdit, "%s", ES_ShortFilename(path));
}

static void
SaveLayoutToGedaPCB(AG_Event *event)
{
	/* TODO */
	AG_TextError("Export to gEDA PCB not implemented yet");
}

static void
SaveLayoutToGerber(AG_Event *event)
{
	/* TODO */
	AG_TextError("Export to Gerber not implemented yet");
}

static void
SaveLayoutToXGerber(AG_Event *event)
{
	/* TODO */
	AG_TextError("Export to XGerber not implemented yet");
}

static void
SaveCircuitToSPICE3(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	char *path = AG_STRING(2);
	
	if (ES_CircuitExportSPICE3(ckt, path) == -1)
		AG_TextMsgFromError();
}

static void
SaveCircuitToTXT(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	char *path = AG_STRING(2);

	if (ES_CircuitExportTXT(ckt, path) == -1)
		AG_TextMsgFromError();
}

static void
SaveSchemToPDF(AG_Event *event)
{
	/* TODO */
	AG_TextError("Export to PDF not implemented yet");
}

static void
SaveAsDlg(AG_Event *event)
{
	char defDir[AG_PATHNAME_MAX];
	AG_Object *obj = AG_PTR(1);
	AG_Window *win;
	AG_FileDlg *fd;
	AG_FileType *ft;

	AG_CopyCfgString("save-path", defDir, sizeof(defDir));

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
		ft = AG_FileDlgAddType(fd, _("SPICE3 netlist"),
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
		AG_FileDlgAddType(fd, _("gEDA PCB format"),
		    "*.pcb",
		    SaveLayoutToGedaPCB, "%p", obj);
		AG_FileDlgAddType(fd, _("Gerber (RS-274D)"),
		    "*.gbr,*.phd,*.spl,*.art",
		    SaveLayoutToGerber, "%p", obj);
		AG_FileDlgAddType(fd, _("Extended Gerber (RS-274X)"),
		    "*.ger,*.gbl,*.gtl,*.gbs,*.gts,*.gbo,*.gto",
		    SaveLayoutToXGerber, "%p", obj);
	} else if (AG_OfClass(obj, "ES_Schem:*")) {
		AG_FileDlgSetDirectoryMRU(fd, "edacious.mru.schems", defDir);
		AG_FileDlgAddType(fd, _("Edacious schematic"),
		    "*.esh",
		    SaveNativeObject, "%p", obj);
		AG_FileDlgAddType(fd, _("Portable Document Format"),
		    "*.pdf",
		    SaveSchemToPDF, "%p", obj);
	}
	AG_WindowShow(win);
}

static void
Save(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);

	if (OBJECT(obj)->archivePath == NULL) {
		SaveAsDlg(event);
		return;
	}
	if (AG_ObjectSave(obj) == -1) {
		AG_TextError(_("Error saving object: %s"), AG_GetError());
	} else {
		AG_TextTmsg(AG_MSG_INFO, 1250, _("Saved %s successfully"),
		    OBJECT(obj)->archivePath);
	}
}

static void
ConfirmQuit(AG_Event *event)
{
	SDL_Event nev;

	nev.type = SDL_USEREVENT;
	SDL_PushEvent(&nev);
}

static void
AbortQuit(AG_Event *event)
{
	AG_Window *win = AG_PTR(1);

	agTerminating = 0;
	AG_ViewDetach(win);
}

static void
Quit(AG_Event *event)
{
	AG_Object *vfsObj = NULL, *modelObj = NULL, *schemObj = NULL;
	AG_Window *win;
	AG_Checkbox *cb;
	AG_Box *box;

	if (agTerminating) {
		ConfirmQuit(NULL);
	}
	agTerminating = 1;

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

			if ((win = AG_WindowNewNamed(AG_WINDOW_MODAL|AG_WINDOW_NOTITLE|
			    AG_WINDOW_NORESIZE, "QuitCallback")) == NULL) {
				return;
			}
			AG_WindowSetCaption(win, _("Quit application?"));
			AG_WindowSetPosition(win, AG_WINDOW_CENTER, 0);
			AG_WindowSetSpacing(win, 8);

			AG_LabelNewString(win, 0, _("Exit Edacious?"));
			cb = AG_CheckboxNew(win, 0, _("Don't ask again"));
			Vdisable = AG_SetInt(agConfig,"no-confirm-quit",0);
			AG_BindInt(cb, "state", &Vdisable->data.i);

			box = AG_BoxNewHorizNS(win, AG_VBOX_HFILL);
			AG_ButtonNewFn(box, 0, _("Quit"), ConfirmQuit, NULL);
			AG_WidgetFocus(AG_ButtonNewFn(box, 0, _("Cancel"), AbortQuit, "%p", win));
			AG_WindowShow(win);
		}
	} else {
		if ((win = AG_WindowNewNamed(AG_WINDOW_MODAL|AG_WINDOW_NOTITLE|
		    AG_WINDOW_NORESIZE, "QuitCallback")) == NULL) {
			return;
		}
		AG_WindowSetCaption(win, _("Exit application?"));
		AG_WindowSetPosition(win, AG_WINDOW_CENTER, 0);
		AG_WindowSetSpacing(win, 8);
		AG_LabelNewString(win, 0,
		    _("There is at least one object with unsaved changes.  "
	              "Exit application?"));
		box = AG_BoxNewHorizNS(win, AG_VBOX_HFILL|AG_BOX_HOMOGENOUS);
		AG_ButtonNewFn(box, 0, _("Discard changes"), ConfirmQuit, NULL);
		AG_WidgetFocus(AG_ButtonNewFn(box, 0, _("Cancel"), AbortQuit, "%p", win));
		AG_WindowShow(win);
	}
}

static void
QuitByKBD(void)
{
	Quit(NULL);
}

static void
VideoResize(Uint w, Uint h)
{
	AG_SetCfgUint("gui-width", w);
	AG_SetCfgUint("gui-height", h);
	(void)AG_ConfigSave();
}

static void
FileMenu(AG_Event *event)
{
	AG_MenuItem *m = AG_SENDER();

	AG_MenuActionKb(m, _("New circuit..."), esIconCircuit.s,
	    SDLK_n, KMOD_CTRL,
	    NewObject, "%p", &esCircuitClass);
	AG_MenuAction(m, _("New component model..."), esIconComponent.s,
	    NewComponentModelDlg, NULL);
	AG_MenuAction(m, _("New schematic..."), vgIconDrawing.s,
	    NewObject, "%p", &esSchemClass);
	AG_MenuAction(m, _("New PCB layout..."), vgIconDrawing.s,
	    NewObject, "%p", &esLayoutClass);
	AG_MenuAction(m, _("New device package..."), vgIconDrawing.s,
	    NewObject, "%p", &esPackageClass);

	AG_MenuActionKb(m, _("Open..."), agIconLoad.s,
	    SDLK_o, KMOD_CTRL,
	    OpenDlg, NULL);

	AG_MutexLock(&objLock);
	if (objFocus == NULL) { AG_MenuDisable(m); }

	AG_MenuActionKb(m, _("Save"), agIconSave.s,
	    SDLK_s, KMOD_CTRL,
	    Save, "%p", objFocus);
	AG_MenuAction(m, _("Save as..."), agIconSave.s,
	    SaveAsDlg, "%p", objFocus);
	
	if (objFocus == NULL) { AG_MenuEnable(m); }
	AG_MutexUnlock(&objLock);
	
	AG_MenuSeparator(m);

#if 0
	node = AG_MenuNode(m, _("Devices"), NULL);
	{
		ES_Device *dev;

		AG_MenuAction(node, _("Microcontroller..."), agIconDoc.s,
		    NewObject, "%p", &esMCU);
		AG_MenuAction(node, _("Oscilloscope..."), agIconDoc.s,
		    NewObject, "%p", &esOscilloscopeClass);
		AG_MenuAction(node, _("Audio input..."), agIconDoc.s,
		    NewObject, "%p", &esAudioInputClass);
		AG_MenuAction(node, _("Audio output..."), agIconDoc.s,
		    NewObject, "%p", &esAudioOutputClass);
		AG_MenuAction(node, _("Digital input..."), agIconDoc.s,
		    NewObject, "%p", &esDigitalInputClass);
		AG_MenuAction(node, _("Digital output..."), agIconDoc.s,
		    NewObject, "%p", &esDigitalOutputClass);

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
	    SDLK_q, KMOD_CTRL,
	    Quit, NULL);
}

static void
Undo(AG_Event *event)
{
	/* TODO */
	printf("undo!\n");
}

static void
Redo(AG_Event *event)
{
	/* TODO */
	printf("redo!\n");
}

static void
EditPreferences(AG_Event *event)
{
	DEV_ConfigShow();
}

static void
SelectedFont(AG_Event *event)
{
	AG_Window *win = AG_PTR(1);

	AG_SetCfgString("font.face", "%s", OBJECT(agDefaultFont)->name);
	AG_SetCfgInt("font.size", agDefaultFont->size);
	AG_SetCfgUint("font.flags", agDefaultFont->flags);
	(void)AG_ConfigSave();

	AG_TextWarning("default-font-changed",
	    _("The default font has been changed.\n"
	      "Please restart Edacious for this change to take effect."));
	AG_ViewDetach(win);
}

static void
SelectFontDlg(AG_Event *event)
{
	AG_Window *win;
	AG_FontSelector *fs;
	AG_Box *hBox;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Font selection"));

	fs = AG_FontSelectorNew(win, AG_FONTSELECTOR_EXPAND);
	AG_WidgetBindPointer(fs, "font", &agDefaultFont);

	hBox = AG_BoxNewHoriz(win, AG_BOX_HFILL|AG_BOX_HOMOGENOUS);
	AG_ButtonNewFn(hBox, 0, _("OK"), SelectedFont, "%p", win);
	AG_ButtonNewFn(hBox, 0, _("Cancel"), AG_WindowCloseGenEv, "%p", win);
	AG_WindowShow(win);
}

static void
EditMenu(AG_Event *event)
{
	AG_MenuItem *m = AG_SENDER();
	
	AG_MutexLock(&objLock);
	if (objFocus == NULL) { AG_MenuDisable(m); }
	AG_MenuActionKb(m, _("Undo"), NULL, SDLK_z, KMOD_CTRL,
	    Undo, "%p", objFocus);
	AG_MenuActionKb(m, _("Redo"), NULL, SDLK_r, KMOD_CTRL,
	    Redo, "%p", objFocus);
	if (objFocus == NULL) { AG_MenuEnable(m); }

	AG_MenuSeparator(m);

	AG_MenuAction(m, _("Preferences..."), NULL, EditPreferences, NULL);
	AG_MenuAction(m, _("Select font..."), NULL, SelectFontDlg, NULL);

	AG_MutexUnlock(&objLock);
}

int
main(int argc, char *argv[])
{
	Uint videoFlags = AG_VIDEO_OPENGL_OR_SDL|AG_VIDEO_RESIZABLE;
	Uint coreFlags = ES_INIT_PRELOAD_ALL;
	int c, i, fps = -1;

#ifdef ENABLE_NLS
	bindtextdomain("edacious", LOCALEDIR);
	bind_textdomain_codeset("edacious", "UTF-8");
	textdomain("edacious");
#endif
	if (AG_InitCore("edacious", AG_VERBOSE|AG_CREATE_DATADIR) == -1) {
		fprintf(stderr, "InitCore: %s\n", AG_GetError());
		return (1);
	}
#ifdef HAVE_GETOPT
	while ((c = getopt(argc, argv, "?vdt:r:T:t:gGP")) != -1) {
		extern char *optarg;

		switch (c) {
		case 'v':
			exit(0);
		case 'd':
			agDebugLvl = 2;
			break;
		case 'r':
			fps = atoi(optarg);
			break;
		case 'T':
			AG_SetString(agConfig, "font-path", optarg);
			break;
		case 't':
			AG_TextParseFontSpec(optarg);
			break;
#ifdef HAVE_OPENGL
		case 'g':
			fprintf(stderr, "Forcing GL mode\n");
			videoFlags &= ~(AG_VIDEO_OPENGL_OR_SDL);
			videoFlags |= AG_VIDEO_OPENGL;
			break;
		case 'G':
			fprintf(stderr, "Forcing SDL mode\n");
			videoFlags &= ~(AG_VIDEO_OPENGL_OR_SDL);
			break;
#endif
		case 'P':
			Verbose("Not preloading modules\n");
			coreFlags &= ~(ES_INIT_PRELOAD_ALL);
			break;
		case '?':
		default:
			printf("Usage: %s [-vdP] [-r fps] "
			       "[-T font-path] [-t fontspec]", agProgName);
#ifdef HAVE_OPENGL
			printf(" [-gG]");
#endif
			printf("\n");
			exit(0);
		}
	}
#endif /* HAVE_GETOPT */

	if (!AG_CfgDefined("gui-width")) { AG_SetCfgUint("gui-width", 640); }
	if (!AG_CfgDefined("gui-height")) { AG_SetCfgUint("gui-height", 480); }

	/* Setup the display. */
	if (AG_InitVideo(
	    AG_CfgUint("gui-width"),
	    AG_CfgUint("gui-height"),
	    32, videoFlags) == -1) {
		fprintf(stderr, "%s\n", AG_GetError());
		return (-1);
	}
	AG_SetRefreshRate(fps);
	AG_SetVideoResizeCallback(VideoResize);
	AG_BindGlobalKey(SDLK_ESCAPE, KMOD_NONE, QuitByKBD);
	AG_BindGlobalKey(SDLK_F8, KMOD_NONE, AG_ViewCapture);

	/*
	 * Initialize the Edacious library. Unless -P was given, we preload
	 * all modules at this point.
	 */
	ES_CoreInit(coreFlags);
	
	/* Configure our editor handlers. */
	ES_SetObjectOpenHandler(OpenObject);
	ES_SetObjectCloseHandler(CloseObject);

	/* Initialize the editor's object list mutex. */
	AG_MutexInitRecursive(&objLock);

	/* Create the application menu. */ 
	appMenu = AG_MenuNewGlobal(0);
	AG_MenuDynamicItem(appMenu->root, _("File"), NULL, FileMenu, NULL);
	AG_MenuDynamicItem(appMenu->root, _("Edit"), NULL, EditMenu, NULL);

#if defined(HAVE_AGAR_DEV) && defined(ES_DEBUG)
	DEV_InitSubsystem(0);
	DEV_ToolMenu(AG_MenuNode(appMenu->root, _("Debug"), NULL));
#endif

#ifdef HAVE_GETOPT
	for (i = optind; i < argc; i++) {
#else
	for (i = 1; i < argc; i++) {
#endif
		AG_Event ev;
		char *ext;

		Verbose("Loading: %s\n", argv[i]);
		if ((ext = strrchr(argv[i], '.')) == NULL)
			continue;

		AG_EventInit(&ev);
		if (strcasecmp(ext, ".ecm") == 0) {
			AG_EventPushPointer(&ev, "", &esCircuitClass);
		} else if (strcasecmp(ext, ".esh") == 0) {
			AG_EventPushPointer(&ev, "", &esSchemClass);
		} else if (strcasecmp(ext, ".ecl") == 0) {
			AG_EventPushPointer(&ev, "", &esLayoutClass);
		} else if (strcasecmp(ext, ".edp") == 0) {
			AG_EventPushPointer(&ev, "", &esPackageClass);
		} else if (strcasecmp(ext, ".em") == 0) {
			AG_EventPushPointer(&ev, "", &esComponentClass);
		} else {
			Verbose("Ignoring argument: %s\n", argv[i]);
			continue;
		}
		AG_EventPushString(&ev, "", argv[i]);
		LoadObject(&ev);
	}

	AG_EventLoop();
	AG_ObjectDestroy(&esVfsRoot);
	AG_Destroy();
	return (0);
}
