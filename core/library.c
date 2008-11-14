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

AG_Object *esModelLibrary = NULL;	/* Component model library */

char **esModelDirs;
int    esModelDirCount;

/* Load a component model from file. */
static int
LoadModelFile(const char *path, AG_Object *objParent)
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
LoadModelsFromDisk(const char *modelDir, AG_Object *objParent)
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
			if (LoadModelsFromDisk(path, objFolder) == -1) {
				AG_Verbose("Ignoring model dir: %s (%s)\n",
				    path, AG_GetError());
			}
			continue;
		}
		if ((s = strstr(file, ".em")) == NULL || s[3] != '\0') {
			continue;
		}
		if (LoadModelFile(path, objParent) == -1)
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
ES_LibraryLoad(void)
{
	int i;

	if (esModelLibrary != NULL) {
		if (AG_ObjectInUse(esModelLibrary)) {
			/* XXX */
			AG_Verbose("Not freeing model library; model(s) are"
			           "currently in use");
		} else {
	//		AG_ObjectDestroy(esModelLibrary);
		}
	}

	/* Create the folder structure. */
	esModelLibrary = AG_ObjectNew(NULL, "Models", &agObjectClass);
	for (i = 0; i < esModelDirCount; i++) {
		if (LoadFoldersFromDisk(esModelDirs[i], esModelLibrary) == -1)
			AG_Verbose("Skipping: %s\n",  AG_GetError());
	}

	/* Load the model files. */
	for (i = 0; i < esModelDirCount; i++) {
		if (LoadModelsFromDisk(esModelDirs[i], esModelLibrary) == -1)
			AG_Verbose("Skipping: %s\n",  AG_GetError());
	}
	return (0);
}

/* Register a search path for model files. */
void
ES_RegisterModelDirectory(const char *path)
{
	char *s, *p;

	esModelDirs = Realloc(esModelDirs,
	    (esModelDirCount+1)*sizeof(char *));
	esModelDirs[esModelDirCount++] = s = Strdup(path);

	if (*(p = &s[strlen(s)-1]) == PATHSEPCHAR)
		*p = '\0';
}

/* Unregister a model directory. */
void
ES_UnregisterModelDirectory(const char *path)
{
	int i;

	for (i = 0; i < esModelDirCount; i++) {
		if (strcmp(esModelDirs[i], path) == 0)
			break;
	}
	if (i == esModelDirCount) {
		return;
	}
	free(esModelDirs[i]);
	if (i < esModelDirCount-1) {
		memmove(&esModelDirs[i], &esModelDirs[i+1],
		    (esModelDirCount-1)*sizeof(char *));
	}
	esModelDirCount--;
}

/* Initialize the model library. */
void
ES_LibraryInit(void)
{
	char path[AG_PATHNAME_MAX];

	esModelLibrary = NULL;
	esModelDirs = NULL;
	esModelDirCount = 0;
	
	Strlcpy(path, SHAREDIR, sizeof(path));
	Strlcat(path, "/Models", sizeof(path));
	printf("Registering: %s\n", path);
	ES_RegisterModelDirectory(path);

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
		ES_RegisterModelDirectory(path);
	}
#endif
	if (ES_LibraryLoad() == -1)
		AG_Verbose("Loading library: %s", AG_GetError());
}

/* Destroy the model library. */
void
ES_LibraryDestroy(void)
{
	AG_ObjectDestroy(esModelLibrary);
	esModelLibrary = NULL;
	Free(esModelDirs);
}

static AG_TlistItem *
FindModels(AG_Tlist *tl, AG_Object *pob, int depth)
{
	AG_Object *chld;
	AG_TlistItem *it;

	it = AG_TlistAddPtr(tl,
	    AG_OfClass(pob, "ES_Circuit:ES_Component:*") ?
	    esIconComponent.s : agIconDirectory.s,
	    pob->name, pob);
	it->depth = depth;

	if (!TAILQ_EMPTY(&pob->children)) {
		it->flags |= AG_TLIST_HAS_CHILDREN;
	}
	if ((it->flags & AG_TLIST_HAS_CHILDREN) &&
	    AG_TlistVisibleChildren(tl, it)) {
		TAILQ_FOREACH(chld, &pob->children, cobjs)
			FindModels(tl, chld, depth+1);
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
	AG_LockVFS(esModelLibrary);

	ti = FindModels(tl, OBJECT(esModelLibrary), 0);
	ti->flags |= AG_TLIST_EXPANDED;

	AG_UnlockVFS(esModelLibrary);
	AG_TlistRestore(tl);
}

static void
InsertComponent(AG_Event *event)
{
	VG_View *vv = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	AG_TlistItem *ti = AG_PTR(3);
	ES_Component *model = ti->p1;
	VG_Tool *insTool;
	
	if (!AG_OfClass(model, "ES_Circuit:ES_Component:*"))
		return;

	if ((insTool = VG_ViewFindTool(vv, "Insert component")) != NULL) {
		VG_ViewSelectTool(vv, insTool, ckt);
		if (ES_InsertComponent(ckt, insTool, model) == -1)
			AG_TextError("Inserting component: %s", AG_GetError());
	}
}

static void
RefreshLibrary(AG_Event *event)
{
	if (ES_LibraryLoad() == -1) {
		AG_TextMsgFromError();
		return;
	}
}

static void
EditModel(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);

	(void)ES_OpenObject(obj);
}

static void
SaveModel(AG_Event *event)
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
SaveModelAsDlg(AG_Event *event)
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
	    SaveModel, "%p", obj);
	AG_WindowShow(win);
}

static void
LibraryModelMenu(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	AG_Object *obj = AG_TlistSelectedItemPtr(tl);
	AG_PopupMenu *pm;

	if (!AG_OfClass(obj, "ES_Circuit:ES_Component:*"))
		return;
	
	pm = AG_PopupNew(tl);

	AG_MenuAction(pm->item, _("Edit model..."), esIconComponent.s,
	    EditModel, "%p", obj);
	AG_MenuSeparator(pm->item);
	AG_MenuAction(pm->item, _("Save"), agIconSave.s,
	    SaveModel, "%p,%s", obj, "");
	AG_MenuAction(pm->item, _("Save as..."), agIconSave.s,
	    SaveModelAsDlg, "%p", obj);
	
	AG_PopupShow(pm);
}

ES_LibraryEditor *
ES_LibraryEditorNew(void *parent, VG_View *vv, ES_Circuit *ckt, Uint flags)
{
	AG_Box *box;
	AG_Button *btn;
	AG_Tlist *tl;

	box = AG_BoxNewVert(parent, AG_BOX_EXPAND);

	tl = AG_TlistNewPolled(box, AG_TLIST_TREE|AG_TLIST_EXPAND,
	    PollLibrary, NULL);
	AG_WidgetSetFocusable(tl, 0);

	AG_TlistSizeHint(tl, "XXXXXXXXXXXXXXXXXXX", 10);
	AG_TlistSetPopupFn(tl, LibraryModelMenu, NULL);
	AG_SetEvent(tl, "tlist-dblclick",
	    InsertComponent, "%p,%p", vv, ckt);

	btn = AG_ButtonNewFn(box, AG_BUTTON_HFILL, _("Refresh list"),
	    RefreshLibrary, NULL);
	AG_WidgetSetFocusable(btn, 0);

	RefreshLibrary(NULL);
	return (ES_LibraryEditor *)box;
}
