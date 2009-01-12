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
 * Interface to the library of component models.
 */

#include "core.h"

#include <string.h>

#include <config/sharedir.h>
#include <config/have_getpwuid.h>
#include <config/have_getuid.h>

#if defined(HAVE_GETPWUID) && defined(HAVE_GETUID)
#include <pwd.h>
#endif

AG_Object *esComponentLibrary = NULL;	/* Component model library */

char **esComponentLibraryDirs;
int    esComponentLibraryDirCount;

/* Load a component model from file. */
static int
LoadComponentFile(const char *path, AG_Object *objParent)
{
	AG_ObjectHeader oh;
	AG_DataSource *ds;
	AG_ObjectClass *cl;
	AG_Object *obj;

	/* Fetch class information from the object file's header. */
	if ((ds = AG_OpenFile(path, "rb")) == NULL) {
		return (-1);
	}
	if (AG_ObjectReadHeader(ds, &oh) == -1) {
		AG_CloseFile(ds);
		return (-1);
	}
	AG_CloseFile(ds);

	Debug(objParent, "%s: Model for %s\n", ES_ShortFilename(path),
	    oh.cs.hier);

	/*
	 * Fetch class information for the model contained. If dynamic library
	 * modules are required, they get linked at this stage.
	 */
	if ((cl = AG_LoadClass(oh.cs.hier)) == NULL)
		return (-1);

	/* Create the component model instance and load its contents. */
	if ((obj = AG_ObjectNew(objParent, NULL, cl)) == NULL) {
		return (-1);
	}
	if (AG_ObjectLoadFromFile(obj, path) == -1) {
		AG_SetError("%s: %s", path, AG_GetError());
		AG_ObjectDelete(obj);
		return (-1);
	}
	ES_SetObjectNameFromPath(obj, path);
	return (0);
}

/* Load component model files in specified directory (and subdirectories). */
static int
LoadComponentsFromDisk(const char *modelDir, AG_Object *objParent)
{
	char path[AG_PATHNAME_MAX];
	AG_Object *objFolder;
	AG_Dir *dir;
	int j;

	Debug(NULL, "Scanning model directory: %s (under \"%s\")\n",
	    modelDir, objParent->name);

	if ((dir = AG_OpenDir(modelDir)) == NULL) {
		return (-1);
	}
	for (j = 0; j < dir->nents; j++) {
		char *file = dir->ents[j];
		AG_FileInfo fileInfo;
		char *s;

		if (file[0] == '.')
			continue;

		Strlcpy(path, modelDir, sizeof(path));
		Strlcat(path, PATHSEP, sizeof(path));
		Strlcat(path, file, sizeof(path));
			
		if (AG_GetFileInfo(path, &fileInfo) == -1) {
			AG_Verbose("Ignoring: %s (%s)\n", path, AG_GetError());
			continue;
		}
		if (fileInfo.type == AG_FILE_DIRECTORY) {
			if ((objFolder = AG_ObjectFindChild(objParent, file))
			    == NULL) {
				/* Should not happen */
				AG_Verbose("Ignoring folder: %s\n", file);
				continue;
			}
			if (LoadComponentsFromDisk(path, objFolder) == -1) {
				AG_Verbose("Ignoring model dir: %s (%s)\n",
				    path, AG_GetError());
			}
			continue;
		}
		if ((s = strstr(file, ".em")) == NULL || s[3] != '\0') {
			continue;
		}
		if (LoadComponentFile(path, objParent) == -1)
			AG_Verbose("Ignoring model file: %s (%s)", path,
			    AG_GetError());
	}
	AG_CloseDir(dir);
	return (0);
}

/* Load component model "folder" objects from disk. */
static int
LoadFoldersFromDisk(const char *modelDir, AG_Object *objParent)
{
	char path[AG_PATHNAME_MAX];
	AG_Dir *dir;
	AG_Object *objFolder;
	int j;

	Debug(NULL, "Scanning for folders in: %s (under \"%s\")\n",
	    modelDir, objParent->name);

	if ((dir = AG_OpenDir(modelDir)) == NULL) {
		return (-1);
	}
	for (j = 0; j < dir->nents; j++) {
		char *file = dir->ents[j];
		AG_FileInfo fileInfo;

		if (file[0] == '.')
			continue;

		Strlcpy(path, modelDir, sizeof(path));
		Strlcat(path, PATHSEP, sizeof(path));
		Strlcat(path, file, sizeof(path));
			
		if (AG_GetFileInfo(path, &fileInfo) == -1) {
			AG_Verbose("Ignoring: %s (%s)\n", path, AG_GetError());
			continue;
		}
		if (fileInfo.type != AG_FILE_DIRECTORY) {
			continue;
		}
		if ((objFolder = AG_ObjectNew(objParent, file, &agObjectClass))
		    == NULL) {
			AG_Verbose("Ignoring folder: %s (%s)\n", path,
			    AG_GetError());
			continue;
		}
		if (LoadFoldersFromDisk(path, objFolder) == -1) {
			AG_Verbose("Ignoring folder: %s (%s)\n", path,
			    AG_GetError());
		}
	}
	AG_CloseDir(dir);
	return (0);
}

/* Load the model library from disk. */
int
ES_ComponentLibraryLoad(void)
{
	int i;

	if (esComponentLibrary != NULL) {
		if (AG_ObjectInUse(esComponentLibrary)) {
			/* XXX */
			AG_Verbose("Not freeing model library; model(s) are"
			           "currently in use");
		} else {
	//		AG_ObjectDestroy(esComponentLibrary);
		}
	}

	/* Create the folder structure. */
	esComponentLibrary = AG_ObjectNew(NULL, "Component Library",
	    &agObjectClass);
	for (i = 0; i < esComponentLibraryDirCount; i++) {
		if (LoadFoldersFromDisk(esComponentLibraryDirs[i], esComponentLibrary) == -1)
			AG_Verbose("Skipping: %s\n",  AG_GetError());
	}

	/* Load the model files. */
	for (i = 0; i < esComponentLibraryDirCount; i++) {
		if (LoadComponentsFromDisk(esComponentLibraryDirs[i],
		    esComponentLibrary) == -1)
			AG_Verbose("Skipping: %s\n",  AG_GetError());
	}
	return (0);
}

/* Register a search path for model files. */
void
ES_ComponentLibraryRegisterDir(const char *path)
{
	char *s, *p;

	esComponentLibraryDirs = Realloc(esComponentLibraryDirs,
	    (esComponentLibraryDirCount+1)*sizeof(char *));
	esComponentLibraryDirs[esComponentLibraryDirCount++] = s = Strdup(path);

	if (*(p = &s[strlen(s)-1]) == PATHSEPCHAR)
		*p = '\0';
}

/* Unregister a model directory. */
void
ES_ComponentLibraryUnregisterDir(const char *path)
{
	int i;

	for (i = 0; i < esComponentLibraryDirCount; i++) {
		if (strcmp(esComponentLibraryDirs[i], path) == 0)
			break;
	}
	if (i == esComponentLibraryDirCount) {
		return;
	}
	free(esComponentLibraryDirs[i]);
	if (i < esComponentLibraryDirCount-1) {
		memmove(&esComponentLibraryDirs[i], &esComponentLibraryDirs[i+1],
		    (esComponentLibraryDirCount-1)*sizeof(char *));
	}
	esComponentLibraryDirCount--;
}

/* Initialize the model library. */
void
ES_ComponentLibraryInit(void)
{
	char path[AG_PATHNAME_MAX];

	esComponentLibrary = NULL;
	esComponentLibraryDirs = NULL;
	esComponentLibraryDirCount = 0;
	
	Strlcpy(path, SHAREDIR, sizeof(path));
	Strlcat(path, "/Models", sizeof(path));
	printf("Registering component library dir: %s\n", path);
	ES_ComponentLibraryRegisterDir(path);

#if defined(HAVE_GETPWUID) && defined(HAVE_GETUID)
	{
		struct passwd *pwd = getpwuid(getuid());
		Strlcpy(path, pwd->pw_dir, sizeof(path));
		Strlcat(path, PATHSEP, sizeof(path));
		Strlcat(path, ".edacious", sizeof(path));
		Strlcat(path, PATHSEP, sizeof(path));
		Strlcat(path, "Models", sizeof(path));
		if (!AG_FileExists(path)) {
			if (AG_MkPath(path) == -1) {
				AG_Verbose("Failed to create %s (%s)\n", path,
				    AG_GetError());
			}
		}
		ES_ComponentLibraryRegisterDir(path);
	}
#endif
	if (ES_ComponentLibraryLoad() == -1)
		AG_Verbose("Loading library: %s", AG_GetError());
}

/* Destroy the model library. */
void
ES_ComponentLibraryDestroy(void)
{
	AG_ObjectDestroy(esComponentLibrary);
	esComponentLibrary = NULL;
	Free(esComponentLibraryDirs);
}

static AG_TlistItem *
FindComponents(AG_Tlist *tl, AG_Object *pob, int depth)
{
	AG_Object *chld;
	AG_TlistItem *it;
	int isComponent = AG_OfClass(pob, "ES_Circuit:ES_Component:*");

	it = AG_TlistAddPtr(tl,
	    isComponent ? esIconComponent.s : agIconDirectory.s,
	    pob->name, pob);
	it->depth = depth;
	it->cat = isComponent ? "component" : "folder";

	if (!TAILQ_EMPTY(&pob->children)) {
		it->flags |= AG_TLIST_HAS_CHILDREN;
	}
	if ((it->flags & AG_TLIST_HAS_CHILDREN) &&
	    AG_TlistVisibleChildren(tl, it)) {
		TAILQ_FOREACH(chld, &pob->children, cobjs)
			FindComponents(tl, chld, depth+1);
	}
	return (it);
}

/* Generate a Tlist tree for the component model library. */
static void
PollLibrary(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	AG_TlistItem *ti;
	
	AG_TlistClear(tl);
	AG_LockVFS(esComponentLibrary);

	ti = FindComponents(tl, OBJECT(esComponentLibrary), 0);
	ti->flags |= AG_TLIST_EXPANDED;

	AG_UnlockVFS(esComponentLibrary);
	AG_TlistRestore(tl);
}

static void
InsertComponent(AG_Event *event)
{
	VG_View *vv = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	AG_TlistItem *ti = AG_PTR(3);
	ES_Component *comModel = ti->p1;
	VG_Tool *insTool;
	
	if (strcmp(ti->cat, "component") != 0)
		return;

	if ((insTool = VG_ViewFindToolByOps(vv, &esComponentInsertTool)) == NULL) {
		AG_TextMsgFromError();
		return;
	}
	VG_ViewSelectTool(vv, insTool, ckt);
	if (VG_ToolCommandExec(insTool, "Insert",
	    "%p,%p,%p", insTool, ckt, comModel) == -1)
		AG_TextMsgFromError();
}

static void
RefreshLibrary(AG_Event *event)
{
	AG_Tlist *tl = AG_PTR(1);

	if (ES_ComponentLibraryLoad() == -1) {
		AG_TextMsgFromError();
		return;
	}
	AG_TlistRefresh(tl);
}

static void
EditComponent(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);

	(void)ES_OpenObject(obj);
}

static void
SaveComponent(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);
	char *path = AG_STRING(2);

	if (path[0] == '\0') {
		if (AG_ObjectSave(obj) == -1) {
			AG_TextMsgFromError();
			return;
		}
		AG_TextTmsg(AG_MSG_INFO, 1250,
		    _("Successfully saved model to %s"),
		    ES_ShortFilename(obj->archivePath));
	} else {
		if (AG_ObjectSaveToFile(obj, path) == -1) {
			AG_TextMsgFromError();
			return;
		}
		ES_SetObjectNameFromPath(obj, path);
		AG_TextTmsg(AG_MSG_INFO, 1250,
		    _("Successfully saved model to %s"),
		    ES_ShortFilename(path));
	}
}

static void
SaveComponentAsDlg(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);
	AG_Window *win;
	AG_FileDlg *fd;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Save %s as..."), obj->name);

	fd = AG_FileDlgNewMRU(win, "edacious.mru.models",
	    AG_FILEDLG_SAVE|AG_FILEDLG_CLOSEWIN|AG_FILEDLG_EXPAND);
	AG_FileDlgSetOptionContainer(fd, AG_BoxNewVert(win, AG_BOX_HFILL));
	AG_FileDlgAddType(fd, _("Edacious Component Model"), "*.em",
	    SaveComponent, "%p", obj);
	AG_WindowShow(win);
}

static void
ComponentMenu(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	AG_Object *obj = AG_TlistSelectedItemPtr(tl);
	AG_PopupMenu *pm;

	if (!AG_OfClass(obj, "ES_Circuit:ES_Component:*"))
		return;
	
	pm = AG_PopupNew(tl);

	AG_MenuAction(pm->item, _("Edit component model..."), esIconComponent.s,
	    EditComponent, "%p", obj);
	AG_MenuSeparator(pm->item);
	AG_MenuAction(pm->item, _("Save"), agIconSave.s,
	    SaveComponent, "%p,%s", obj, "");
	AG_MenuAction(pm->item, _("Save as..."), agIconSave.s,
	    SaveComponentAsDlg, "%p", obj);
	
	AG_PopupShow(pm);
}

ES_ComponentLibraryEditor *
ES_ComponentLibraryEditorNew(void *parent, VG_View *vv, ES_Circuit *ckt,
    Uint flags)
{
	AG_Box *box;
	AG_Button *btn;
	AG_Tlist *tl;
	AG_Event ev;

	box = AG_BoxNewVert(parent, AG_BOX_EXPAND);

	tl = AG_TlistNewPolled(box, AG_TLIST_TREE|AG_TLIST_EXPAND,
	    PollLibrary, NULL);
	AG_TlistSetRefresh(tl, -1);
	AG_WidgetSetFocusable(tl, 0);

	AG_TlistSizeHint(tl, "XXXXXXXXXXXXXXXXXXX", 10);
	AG_TlistSetPopupFn(tl, ComponentMenu, NULL);
	AG_SetEvent(tl, "tlist-dblclick",
	    InsertComponent, "%p,%p", vv, ckt);

	btn = AG_ButtonNewFn(box, AG_BUTTON_HFILL, _("Refresh list"),
	    RefreshLibrary, "%p", tl);
	AG_WidgetSetFocusable(btn, 0);

	AG_EventArgs(&ev, "%p", tl);
	RefreshLibrary(&ev);
	return (ES_ComponentLibraryEditor *)box;
}
