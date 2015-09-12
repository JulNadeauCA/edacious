/*
 * Copyright (c) 2009-2015 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Interface to the library of device packages.
 */

#include "core.h"

#include <string.h>

#include <config/datadir.h>
#include <config/have_getpwuid.h>
#include <config/have_getuid.h>

#if defined(HAVE_GETPWUID) && defined(HAVE_GETUID)
#include <pwd.h>
#endif

AG_Object *esPackageLibrary = NULL;	/* Package library */

char **esPackageLibraryDirs;
int    esPackageLibraryDirCount;

/* Load a package from file. */
static int
LoadPackageFile(const char *path, AG_Object *objParent)
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

	/*
	 * Fetch class information for the model contained. If dynamic library
	 * modules are required, they get linked at this stage.
	 */
	if ((cl = AG_LoadClass(oh.cs.hier)) == NULL)
		return (-1);

	/* Create the package model instance and load its contents. */
	if ((obj = AG_ObjectNew(objParent, NULL, cl)) == NULL) {
		return (-1);
	}
	if (AG_ObjectLoadFromFile(obj, path) == -1) {
		AG_SetError("%s: %s", path, AG_GetError());
		AG_ObjectDelete(obj);
		return (-1);
	}
	AG_ObjectSetArchivePath(obj, path);
	AG_ObjectSetNameS(obj, AG_ShortFilename(path));
	return (0);
}

/* Load package model files in specified directory (and subdirectories). */
static int
LoadPackagesFromDisk(const char *modelDir, AG_Object *objParent)
{
	char path[AG_PATHNAME_MAX];
	AG_Object *objFolder;
	AG_Dir *dir;
	int j;
#if 0
	Debug(NULL, "Scanning model directory: %s (under \"%s\")\n",
	    modelDir, objParent->name);
#endif
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
			if (LoadPackagesFromDisk(path, objFolder) == -1) {
				AG_Verbose("Ignoring model dir: %s (%s)\n",
				    path, AG_GetError());
			}
			continue;
		}
		if ((s = strstr(file, ".em")) == NULL || s[3] != '\0') {
			continue;
		}
		if (LoadPackageFile(path, objParent) == -1)
			AG_Verbose("Ignoring model file: %s (%s)", path,
			    AG_GetError());
	}
	AG_CloseDir(dir);
	return (0);
}

/* Load package model "folder" objects from disk. */
static int
LoadFoldersFromDisk(const char *modelDir, AG_Object *objParent)
{
	char path[AG_PATHNAME_MAX];
	AG_Dir *dir;
	AG_Object *objFolder;
	int j;
#if 0
	Debug(NULL, "Scanning for folders in: %s (under \"%s\")\n",
	    modelDir, objParent->name);
#endif
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
ES_PackageLibraryLoad(void)
{
	int i;

	if (esPackageLibrary != NULL) {
		if (AG_ObjectInUse(esPackageLibrary)) {
			/* XXX */
			AG_Verbose("Not freeing model library; model(s) are"
			           "currently in use");
		} else {
	//		AG_ObjectDestroy(esPackageLibrary);
		}
	}

	/* Create the folder structure. */
	esPackageLibrary = AG_ObjectNew(NULL, "Package Library", &agObjectClass);
	for (i = 0; i < esPackageLibraryDirCount; i++) {
		if (LoadFoldersFromDisk(esPackageLibraryDirs[i], esPackageLibrary) == -1)
			AG_Verbose("Skipping: %s\n",  AG_GetError());
	}

	/* Load the model files. */
	for (i = 0; i < esPackageLibraryDirCount; i++) {
		if (LoadPackagesFromDisk(esPackageLibraryDirs[i],
		    esPackageLibrary) == -1)
			AG_Verbose("Skipping: %s\n",  AG_GetError());
	}
	return (0);
}

/* Register a search path for model files. */
void
ES_PackageLibraryRegisterDir(const char *path)
{
	char *s, *p;

	esPackageLibraryDirs = Realloc(esPackageLibraryDirs,
	    (esPackageLibraryDirCount+1)*sizeof(char *));
	esPackageLibraryDirs[esPackageLibraryDirCount++] = s = Strdup(path);

	if (*(p = &s[strlen(s)-1]) == PATHSEPCHAR)
		*p = '\0';
}

/* Unregister a model directory. */
void
ES_PackageLibraryUnregisterDir(const char *path)
{
	int i;

	for (i = 0; i < esPackageLibraryDirCount; i++) {
		if (strcmp(esPackageLibraryDirs[i], path) == 0)
			break;
	}
	if (i == esPackageLibraryDirCount) {
		return;
	}
	free(esPackageLibraryDirs[i]);
	if (i < esPackageLibraryDirCount-1) {
		memmove(&esPackageLibraryDirs[i], &esPackageLibraryDirs[i+1],
		    (esPackageLibraryDirCount-i-1)*sizeof(char *));
	}
	esPackageLibraryDirCount--;
}

/* Initialize the model library. */
void
ES_PackageLibraryInit(void)
{
	char path[AG_PATHNAME_MAX];

	esPackageLibrary = NULL;
	esPackageLibraryDirs = NULL;
	esPackageLibraryDirCount = 0;
	
	Strlcpy(path, DATADIR, sizeof(path));
	Strlcat(path, "/Packages", sizeof(path));
	printf("Registering package library dir: %s\n", path);
	ES_PackageLibraryRegisterDir(path);

#if defined(HAVE_GETPWUID) && defined(HAVE_GETUID)
	{
		struct passwd *pwd = getpwuid(getuid());
		Strlcpy(path, pwd->pw_dir, sizeof(path));
		Strlcat(path, PATHSEP, sizeof(path));
		Strlcat(path, ".edacious", sizeof(path));
		Strlcat(path, PATHSEP, sizeof(path));
		Strlcat(path, "Packages", sizeof(path));
		if (!AG_FileExists(path)) {
			if (AG_MkPath(path) == -1) {
				AG_Verbose("Failed to create %s (%s)\n", path,
				    AG_GetError());
			}
		}
		ES_PackageLibraryRegisterDir(path);
	}
#endif
	if (ES_PackageLibraryLoad() == -1)
		AG_Verbose("Loading library: %s", AG_GetError());
}

/* Destroy the model library. */
void
ES_PackageLibraryDestroy(void)
{
	AG_ObjectDestroy(esPackageLibrary);
	esPackageLibrary = NULL;
	Free(esPackageLibraryDirs);
}

static AG_TlistItem *
FindPackages(AG_Tlist *tl, AG_Object *pob, int depth)
{
	AG_Object *chld;
	AG_TlistItem *it;

	it = AG_TlistAddPtr(tl,
	    AG_OfClass(pob, "ES_Layout:ES_Package:*") ?
	    esIconComponent.s : agIconDirectory.s,
	    pob->name, pob);
	it->depth = depth;

	if (!TAILQ_EMPTY(&pob->children)) {
		it->flags |= AG_TLIST_HAS_CHILDREN;
	}
	if ((it->flags & AG_TLIST_HAS_CHILDREN) &&
	    AG_TlistVisibleChildren(tl, it)) {
		TAILQ_FOREACH(chld, &pob->children, cobjs)
			FindPackages(tl, chld, depth+1);
	}
	return (it);
}

/* Generate a Tlist tree for the package library. */
static void
PollLibrary(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	AG_TlistItem *ti;
	
	AG_TlistClear(tl);
	AG_LockVFS(esPackageLibrary);

	ti = FindPackages(tl, OBJECT(esPackageLibrary), 0);
	ti->flags |= AG_TLIST_EXPANDED;

	AG_UnlockVFS(esPackageLibrary);
	AG_TlistRestore(tl);
}

static void
RefreshLibrary(AG_Event *event)
{
	AG_Tlist *tl = AG_PTR(1);

	if (ES_PackageLibraryLoad() == -1) {
		AG_TextMsgFromError();
		return;
	}
	AG_TlistRefresh(tl);
}

static void
EditPackage(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);

	(void)ES_OpenObject(obj);
}

static void
SavePackage(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);
	char *path = AG_STRING(2);
	const char *sname;

	if (path[0] == '\0') {
		sname = AG_ShortFilename(obj->archivePath);
		if (AG_ObjectSave(obj) == -1) {
			AG_TextMsgFromError();
			return;
		}
		AG_TextTmsg(AG_MSG_INFO, 1250,
		    _("Successfully saved package model to %s"),
		    sname);
	} else {
		sname = AG_ShortFilename(path);
		if (AG_ObjectSaveToFile(obj, path) == -1) {
			AG_TextMsgFromError();
			return;
		}
		AG_ObjectSetArchivePath(obj, path);
		AG_ObjectSetNameS(obj, sname);
		AG_TextTmsg(AG_MSG_INFO, 1250,
		    _("Successfully saved package model to %s"), sname);
	}
}

static void
SavePackageAsDlg(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);
	AG_Window *win;
	AG_FileDlg *fd;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Save %s as..."), obj->name);

	fd = AG_FileDlgNewMRU(win, "edacious.mru.packages",
	    AG_FILEDLG_SAVE|AG_FILEDLG_CLOSEWIN|AG_FILEDLG_EXPAND);
	AG_FileDlgSetOptionContainer(fd, AG_BoxNewVert(win, AG_BOX_HFILL));
	AG_FileDlgAddType(fd, _("Edacious device package"), "*.edp",
	    SavePackage, "%p", obj);
	AG_WindowShow(win);
}

static void
PackageMenu(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	AG_Object *obj = AG_TlistSelectedItemPtr(tl);
	AG_PopupMenu *pm;

	if (!AG_OfClass(obj, "ES_Layout:ES_Package:*"))
		return;

	if ((pm = AG_PopupNew(tl)) != NULL)
		return;

	AG_MenuAction(pm->root, _("Edit package model..."), esIconComponent.s,
	    EditPackage, "%p", obj);

	AG_MenuSeparator(pm->root);

	AG_MenuAction(pm->root, _("Save"),       agIconSave.s, SavePackage, "%p,%s", obj, "");
	AG_MenuAction(pm->root, _("Save as..."), agIconSave.s, SavePackageAsDlg, "%p", obj);
	
	AG_PopupShow(pm);
}

ES_PackageLibraryEditor *
ES_PackageLibraryEditorNew(void *parent, VG_View *vv, ES_Layout *lo,
    Uint flags, AG_EventFn insFn, const char *fmt, ...)
{
	AG_Box *box;
	AG_Button *btn;
	AG_Tlist *tl;
	AG_Event ev;
	AG_Event *evSel;

	box = AG_BoxNewVert(parent, AG_BOX_EXPAND);

	tl = AG_TlistNewPolled(box, AG_TLIST_TREE|AG_TLIST_EXPAND,
	    PollLibrary, NULL);
	AG_WidgetSetFocusable(tl, 0);
	AG_TlistSetRefresh(tl, -1);
	AG_TlistSizeHint(tl, "XXXXXXXXXXXXXXXXXXX", 10);
	AG_TlistSetPopupFn(tl, PackageMenu, NULL);
	evSel = AG_SetEvent(tl, "tlist-dblclick", insFn, NULL);
	AG_EVENT_GET_ARGS(evSel, fmt);

	btn = AG_ButtonNewFn(box, AG_BUTTON_HFILL, _("Refresh list"),
	    RefreshLibrary, "%p", tl);
	AG_WidgetSetFocusable(btn, 0);

	AG_EventArgs(&ev, "%p", tl);
	RefreshLibrary(&ev);
	return (ES_PackageLibraryEditor *)box;
}
