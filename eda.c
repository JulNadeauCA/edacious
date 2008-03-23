/*
 * Copyright (c) 2003-2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Main user interface code for Agar-EDA.
 */

#include <agar/core.h>
#include <agar/vg.h>
#include <agar/gui.h>
#include <agar/dev.h>
#include <agar/sc.h>

#include "eda.h"

#include <string.h>
#include <unistd.h>

#include <circuit/circuit.h>
#include <circuit/scope.h>
#include <circuit/spice.h>

#include <config/have_getopt.h>

extern ES_ComponentClass esDigitalClass;
extern ES_ComponentClass esVsourceClass;
extern ES_ComponentClass esGroundClass;
extern ES_ComponentClass esResistorClass;
extern ES_ComponentClass esSemiResistorClass;
extern ES_ComponentClass esInverterClass;
extern ES_ComponentClass esAndClass;
extern ES_ComponentClass esOrClass;
extern ES_ComponentClass esSpstClass;
extern ES_ComponentClass esSpdtClass;
extern ES_ComponentClass esLedClass;
extern ES_ComponentClass esLogicProbeClass;
extern ES_ComponentClass esVSquareClass;
extern ES_ComponentClass esVSineClass;

void *edaModels[] = {
	&esVsourceClass,
	&esGroundClass,
	&esResistorClass,
	&esSemiResistorClass,
	&esSpstClass,
	&esSpdtClass,
	&esLedClass,
	&esLogicProbeClass,
	&esInverterClass,
	&esAndClass,
	&esOrClass,
	&esVSquareClass,
	&esVSineClass,
	NULL
};

AG_Menu *appMenu = NULL;
AG_Object vfsRoot;
static void *objFocus = NULL;
static AG_Mutex objLock;

static void
RegisterClasses(void)
{
	void **model;

	AG_RegisterClass(&esComponentClass);
	AG_RegisterClass(&esDigitalClass);
	AG_RegisterClass(&esCircuitClass);
	AG_RegisterClass(&esScopeClass);
	for (model = &edaModels[0]; *model != NULL; model++) {
		AG_RegisterClass(*model);
	}
}

static void
SaveAndClose(AG_Object *obj, AG_Window *win)
{
	AG_ViewDetach(win);
	AG_ObjectPageOut(obj);
}

static void
SaveChangesReturn(AG_Event *event)
{
	SaveAndClose(AG_PTR(2), AG_PTR(1));
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
		    _("Save changes to %s?"), AGOBJECT(obj)->name);
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
	
	AG_MutexLock(&objLock);
	if (AG_ObjectIsClass(obj, "ES_Circuit:*") ||
	    AG_ObjectIsClass(obj, "ES_Scope:*")) {
		objFocus = obj;
	} else {
		objFocus = NULL;
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

AG_Window *
ES_CreateEditionWindow(void *p)
{
	AG_Object *obj = p;
	AG_Window *win;

	win = obj->cls->edit(obj);
	AG_SetEvent(win, "window-close", SaveChangesDlg, "%p", obj);
	AG_AddEvent(win, "window-gainfocus", WindowGainedFocus, "%p", obj);
	AG_AddEvent(win, "window-lostfocus", WindowLostFocus, "%p", obj);
	AG_AddEvent(win, "window-hidden", WindowLostFocus, "%p", obj);
	AG_WindowShow(win);
	return (win);
}

static void
NewObject(AG_Event *event)
{
	AG_ObjectClass *cls = AG_PTR(1);

	ES_CreateEditionWindow(AG_ObjectNew(&vfsRoot, NULL, cls));
}

static void
EditDevice(AG_Event *event)
{
	AG_Object *dev = AG_PTR(1);

	ES_CreateEditionWindow(dev);
}

static void
SetArchivePath(void *obj, const char *path)
{
	const char *c;

	AG_ObjectSetArchivePath(obj, path);

	if ((c = strrchr(path, ES_PATHSEPC)) != NULL && c[1] != '\0') {
		AG_ObjectSetName(obj, "%s", &c[1]);
	} else {
		AG_ObjectSetName(obj, "%s", path);
	}
}

/* Load a circuit file from native agar-eda format. */
static void
OpenCircuit(AG_Event *event)
{
	char *path = AG_STRING(1);
	AG_Object *obj;

	obj = AG_ObjectNew(&vfsRoot, NULL, &esCircuitClass);
	if (AG_ObjectLoadFromFile(obj, path) == -1) {
		AG_TextMsgFromError();
		AG_ObjectDetach(obj);
		AG_ObjectDestroy(obj);
		return;
	}
	SetArchivePath(obj, path);
	ES_CreateEditionWindow(obj);
}

static void
OpenDlg(AG_Event *event)
{
	AG_Window *win;
	AG_FileDlg *fd;
	AG_FileType *ft;
	AG_Pane *hPane;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Open..."));

	hPane = AG_PaneNewHoriz(win, AG_PANE_EXPAND);
	fd = AG_FileDlgNewMRU(hPane->div[0], "agar-eda.mru.circuits",
	    AG_FILEDLG_LOAD|AG_FILEDLG_ASYNC|AG_FILEDLG_EXPAND);
	AG_FileDlgSetOptionContainer(fd, hPane->div[1]);

	AG_FileDlgAddType(fd, _("Agar-EDA circuit model"), "*.ecm",
	    OpenCircuit, NULL);

	AG_WindowShow(win);
	AG_WindowSetGeometry(win, AGWIDGET(win)->x-100, AGWIDGET(win)->y,
	                          AGWIDGET(win)->w+200, AGWIDGET(win)->h);
}

static void
SaveCircuitToECM(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	char *path = AG_STRING(2);

	if (AG_ObjectSaveToFile(ckt, path) == -1) {
		AG_TextMsgFromError();
	}
	SetArchivePath(ckt, path);
}

static void
SaveCircuitToECL(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	char *path = AG_STRING(2);

	if (AG_ObjectSaveToFile(ckt, path) == -1) {
		AG_TextMsgFromError();
	}
	SetArchivePath(ckt, path);
}

static void
SaveCircuitToGerber(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	char *path = AG_STRING(2);

	AG_TextMsg(AG_MSG_ERROR, "Export to Gerber not implemented yet");
}

static void
SaveCircuitToXGerber(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	char *path = AG_STRING(2);

	AG_TextMsg(AG_MSG_ERROR, "Export to XGerber not implemented yet");
}

static void
SaveCircuitToSPICE3(AG_Event *event)
{
	char name[FILENAME_MAX];
	ES_Circuit *ckt = AG_PTR(1);
	char *path = AG_STRING(2);
	
	if (ES_CircuitExportSPICE3(ckt, path) == -1)
		AG_TextMsg(AG_MSG_ERROR, "%s: %s", AGOBJECT(ckt)->name,
		    AG_GetError());
}

static void
SaveCircuitToPDF(AG_Event *event)
{
	char name[FILENAME_MAX];
	ES_Circuit *ckt = AG_PTR(1);
	char *path = AG_STRING(2);

	/* TODO */
	AG_TextMsg(AG_MSG_ERROR, "Export to PDF not implemented yet");
}

static void
SaveAsDlg(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);
	AG_Window *win;
	AG_FileDlg *fd;
	AG_FileType *ft;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Save %s as..."), obj->name);
	fd = AG_FileDlgNewMRU(win, "agar-eda.mru.circuits",
	    AG_FILEDLG_SAVE|AG_FILEDLG_CLOSEWIN|AG_FILEDLG_EXPAND);
	AG_FileDlgSetOptionContainer(fd, AG_BoxNewVert(win, AG_BOX_HFILL));

	if (AG_ObjectIsClass(obj, "ES_Circuit:*")) {
		AG_FileDlgAddType(fd, _("Agar-EDA circuit model"), "*.ecm",
		    SaveCircuitToECM, "%p", obj);
		ft = AG_FileDlgAddType(fd, _("SPICE3 netlist"), "*.cir",
		    SaveCircuitToSPICE3, "%p", obj);
		AG_FileDlgAddType(fd, _("Portable Document Format"), "*.pdf",
		    SaveCircuitToPDF, "%p", obj);
		/* ... */
	} else if (AG_ObjectIsClass(obj, "ES_Layout:*")) {
		AG_FileDlgAddType(fd, _("Agar-EDA circuit layout"), "*.ecl",
		    SaveCircuitToECL, "%p", obj);
		AG_FileDlgAddType(fd, _("Gerber (RS-274D)"),
		    "*.gbr,*.phd,*.spl,*.art",
		    SaveCircuitToGerber, "%p", obj);
		AG_FileDlgAddType(fd, _("Extended Gerber (RS-274X)"),
		    "*.gbl,*.gtl,*.gbs,*.gts,*.gbo,*.gto",
		    SaveCircuitToXGerber, "%p", obj);
	}
	AG_WindowShow(win);
}

static void
Save(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);

	if (AGOBJECT(obj)->archivePath == NULL) {
		SaveAsDlg(event);
		return;
	}
	if (AG_ObjectSave(obj) == -1) {
		AG_TextMsg(AG_MSG_ERROR, _("Error saving object: %s"),
		    AG_GetError());
	} else {
		AG_TextInfo(_("Saved object %s successfully"),
		    AGOBJECT(obj)->name);
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
	AG_Object *obj;
	AG_Window *win;
	AG_Box *box;

	if (agTerminating) {
		ConfirmQuit(NULL);
	}
	agTerminating = 1;

	AGOBJECT_FOREACH_CHILD(obj, &vfsRoot, ag_object) {
		if (AG_ObjectChanged(obj))
			break;
	}
	if (obj == NULL) {
		ConfirmQuit(NULL);
	} else {
		if ((win = AG_WindowNewNamed(AG_WINDOW_MODAL|AG_WINDOW_NOTITLE|
		    AG_WINDOW_NORESIZE, "QuitCallback")) == NULL) {
			return;
		}
		AG_WindowSetCaption(win, _("Exit application?"));
		AG_WindowSetPosition(win, AG_WINDOW_CENTER, 0);
		AG_WindowSetSpacing(win, 8);
		AG_LabelNewStaticString(win, 0,
		    _("There is at least one object with unsaved changes.  "
	              "Exit application?"));
		box = AG_BoxNew(win, AG_BOX_HORIZ, AG_BOX_HOMOGENOUS|
		                                   AG_VBOX_HFILL);
		AG_BoxSetSpacing(box, 0);
		AG_BoxSetPadding(box, 0);
		AG_ButtonNewFn(box, 0, _("Discard changes"),
		    ConfirmQuit, NULL);
		AG_WidgetFocus(AG_ButtonNewFn(box, 0, _("Cancel"),
		    AbortQuit, "%p", win));
		AG_WindowShow(win);
	}
}

static void
FileMenu(AG_Event *event)
{
	AG_MenuItem *m = AG_SENDER();
	AG_MenuItem *node;

	AG_MenuActionKb(m, _("New circuit..."), agIconDoc.s, SDLK_s, KMOD_ALT,
	    NewObject, "%p", &esCircuitClass);

	AG_MenuActionKb(m, _("Open..."), agIconLoad.s, SDLK_o, KMOD_CTRL,
	    OpenDlg, NULL);

	AG_MutexLock(&objLock);
	if (objFocus == NULL) { AG_MenuDisable(m); }

	AG_MenuActionKb(m, _("Save"), agIconSave.s, SDLK_s, KMOD_CTRL,
	    Save, "%p", objFocus);
	AG_MenuAction(m, _("Save as..."), agIconSave.s,
	    SaveAsDlg, "%p", objFocus);
	
	if (objFocus == NULL) { AG_MenuEnable(m); }
	AG_MutexUnlock(&objLock);
	
	AG_MenuSeparator(m);

#if 0
	node = AG_MenuNode(m, _("Devices"), NULL);
	{
		ES_Device *mach;

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
		AGOBJECT_FOREACH_CLASS(mach, &vfsRoot, cam_machine,
		    "ES_Device:*") {
			AG_MenuAction(node, AGOBJECT(mach)->name, agIconGear.s,
			    EditDevice, "%p", mach);
		}
	}
#endif
	
	AG_MenuSeparator(m);
	
	AG_MenuActionKb(m, _("Quit"), NULL, SDLK_q, KMOD_CTRL, Quit, NULL);
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
	AG_MutexUnlock(&objLock);
}

int
main(int argc, char *argv[])
{
	int c, i, fps = -1;
	char *s;

#ifdef ENABLE_NLS
	bindtextdomain("agar-eda", LOCALEDIR);
	bind_textdomain_codeset("agar-eda", "UTF-8");
	textdomain("agar-eda");
#endif
	if (AG_InitCore("agar-eda", 0) == -1) {
		fprintf(stderr, "%s\n", AG_GetError());
		return (1);
	}
	AG_TextParseFontSpec("Vera.ttf:15");
#ifdef HAVE_GETOPT
	while ((c = getopt(argc, argv, "?vt:r:T:t:gG")) != -1) {
		extern char *optarg;

		switch (c) {
		case 'v':
			exit(0);
		case 'r':
			fps = atoi(optarg);
			break;
		case 'T':
			AG_SetString(agConfig, "font-path", "%s", optarg);
			break;
		case 't':
			AG_TextParseFontSpec(optarg);
			break;
#ifdef HAVE_OPENGL
		case 'g':
			AG_SetBool(agConfig, "view.opengl", 1);
			break;
		case 'G':
			AG_SetBool(agConfig, "view.opengl", 0);
			break;
#endif
		case '?':
		default:
			printf("Usage: %s [-v] [-r fps] [-T font-path] "
			       "[-t fontspec]", agProgName);
#ifdef HAVE_OPENGL
			printf(" [-gG]");
#endif
			printf("\n");
			exit(0);
		}
	}
#endif /* HAVE_GETOPT */
	if (AG_InitVideo(1024, 768, 32, AG_VIDEO_RESIZABLE) == -1) {
		fprintf(stderr, "%s\n", AG_GetError());
		return (-1);
	}
	VG_InitSubsystem();
	SC_InitSubsystem();
	AG_SetRefreshRate(fps);
	AG_BindGlobalKey(SDLK_ESCAPE, KMOD_NONE, AG_Quit);
	AG_BindGlobalKey(SDLK_F8, KMOD_NONE, AG_ViewCapture);

	/* Initialize our editor VFS. */
	AG_ObjectInitStatic(&vfsRoot, NULL);
	AG_ObjectSetName(&vfsRoot, _("Editor VFS"));
	AG_MutexInit(&objLock);

	/* Register our classes. */
	RegisterClasses();
	
	/* Create the application menu. */ 
	appMenu = AG_MenuNewGlobal(0);
	AG_MenuDynamicItem(appMenu->root, _("File"), NULL, FileMenu, NULL);
	AG_MenuDynamicItem(appMenu->root, _("Edit"), NULL, EditMenu, NULL);

#if defined(HAVE_AGAR_DEV) && defined(DEBUG)
	/* Create an Agar-DEV browser for debugging our VFS */
	DEV_InitSubsystem(0);
	if (agDebugLvl >= 5) {
		DEV_Browser(&vfsRoot);
	}
	DEV_ToolMenu(AG_MenuNode(appMenu->root, _("Debug"), NULL));
#endif

	AG_EventLoop();
	AG_ObjectDestroy(&vfsRoot);
	AG_Destroy();
	return (0);
}

