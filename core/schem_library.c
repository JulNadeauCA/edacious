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
 * Interface to the library of generic schematic blocks.
 */

#include "core.h"

#include <string.h>

#include <config/sharedir.h>
#include <config/have_getpwuid.h>
#include <config/have_getuid.h>

#if defined(HAVE_GETPWUID) && defined(HAVE_GETUID)
#include <pwd.h>
#endif

AG_Object *esSchemLibrary = NULL;	/* Schematic block library */
char **esSchemLibraryDirs;		/* Search directories */
int    esSchemLibraryDirCount;

/* Load a schem from file. */
static int
LoadSchemFile(const char *path, AG_Object *objParent)
{
	AG_ObjectHeader oh;
	AG_DataSource *ds;
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

	if ((obj = AG_ObjectNew(objParent, NULL, &esSchemClass)) == NULL) {
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

/* Load schem files in specified directory (and subdirectories). */
static int
LoadSchemsFromDisk(const char *schemDir, AG_Object *objParent)
{
	char path[AG_PATHNAME_MAX];
	AG_Object *objFolder;
	AG_Dir *dir;
	int j;
#if 0
	Debug(NULL, "Scanning schem directory: %s (under \"%s\")\n",
	    schemDir, objParent->name);
#endif
	if ((dir = AG_OpenDir(schemDir)) == NULL) {
		return (-1);
	}
	for (j = 0; j < dir->nents; j++) {
		char *file = dir->ents[j];
		AG_FileInfo fileInfo;
		char *s;

		if (file[0] == '.')
			continue;

		Strlcpy(path, schemDir, sizeof(path));
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
			if (LoadSchemsFromDisk(path, objFolder) == -1) {
				AG_Verbose("Ignoring schem dir: %s (%s)\n",
				    path, AG_GetError());
			}
			continue;
		}
		if ((s = strstr(file, ".esh")) == NULL || s[4] != '\0') {
			continue;
		}
		if (LoadSchemFile(path, objParent) == -1)
			AG_Verbose("Ignoring schem file: %s (%s)", path,
			    AG_GetError());
	}
	AG_CloseDir(dir);
	return (0);
}

/* Load schem "folder" objects from disk. */
static int
LoadFoldersFromDisk(const char *schemDir, AG_Object *objParent)
{
	char path[AG_PATHNAME_MAX];
	AG_Dir *dir;
	AG_Object *objFolder;
	int j;
#if 0
	Debug(NULL, "Scanning for folders in: %s (under \"%s\")\n",
	    schemDir, objParent->name);
#endif
	if ((dir = AG_OpenDir(schemDir)) == NULL) {
		return (-1);
	}
	for (j = 0; j < dir->nents; j++) {
		char *file = dir->ents[j];
		AG_FileInfo fileInfo;

		if (file[0] == '.')
			continue;

		Strlcpy(path, schemDir, sizeof(path));
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

/* Load the schem library from disk. */
int
ES_SchemLibraryLoad(void)
{
	int i;

	if (esSchemLibrary != NULL) {
		if (AG_ObjectInUse(esSchemLibrary)) {
			/* XXX */
			AG_Verbose("Not freeing schem library; schem(s) are"
			           "currently in use");
		} else {
	//		AG_ObjectDestroy(esSchemLibrary);
		}
	}

	/* Create the folder structure. */
	esSchemLibrary = AG_ObjectNew(NULL, "Schematic library", &agObjectClass);
	for (i = 0; i < esSchemLibraryDirCount; i++) {
		if (LoadFoldersFromDisk(esSchemLibraryDirs[i], esSchemLibrary) == -1)
			AG_Verbose("Skipping: %s\n",  AG_GetError());
	}

	/* Load the schem files. */
	for (i = 0; i < esSchemLibraryDirCount; i++) {
		if (LoadSchemsFromDisk(esSchemLibraryDirs[i], esSchemLibrary) == -1)
			AG_Verbose("Skipping: %s\n",  AG_GetError());
	}
	return (0);
}

/* Register a search path for schem files. */
void
ES_SchemLibraryRegisterDir(const char *path)
{
	char *s, *p;

	esSchemLibraryDirs = Realloc(esSchemLibraryDirs,
	    (esSchemLibraryDirCount+1)*sizeof(char *));
	esSchemLibraryDirs[esSchemLibraryDirCount++] = s = Strdup(path);

	if (*(p = &s[strlen(s)-1]) == PATHSEPCHAR)
		*p = '\0';
}

/* Unregister a schem directory. */
void
ES_SchemLibraryUnregisterDir(const char *path)
{
	int i;

	for (i = 0; i < esSchemLibraryDirCount; i++) {
		if (strcmp(esSchemLibraryDirs[i], path) == 0)
			break;
	}
	if (i == esSchemLibraryDirCount) {
		return;
	}
	free(esSchemLibraryDirs[i]);
	if (i < esSchemLibraryDirCount-1) {
		memmove(&esSchemLibraryDirs[i], &esSchemLibraryDirs[i+1],
		    (esSchemLibraryDirCount-1)*sizeof(char *));
	}
	esSchemLibraryDirCount--;
}

/* Initialize the schem library. */
void
ES_SchemLibraryInit(void)
{
	char path[AG_PATHNAME_MAX];

	esSchemLibrary = NULL;
	esSchemLibraryDirs = NULL;
	esSchemLibraryDirCount = 0;
	
	Strlcpy(path, SHAREDIR, sizeof(path));
	Strlcat(path, "/Schematics", sizeof(path));
	ES_SchemLibraryRegisterDir(path);

#if defined(HAVE_GETPWUID) && defined(HAVE_GETUID)
	{
		struct passwd *pwd = getpwuid(getuid());
		Strlcpy(path, pwd->pw_dir, sizeof(path));
		Strlcat(path, PATHSEP, sizeof(path));
		Strlcat(path, ".edacious", sizeof(path));
		Strlcat(path, PATHSEP, sizeof(path));
		Strlcat(path, "Schematics", sizeof(path));
		if (!AG_FileExists(path)) {
			if (AG_MkPath(path) == -1) {
				AG_Verbose("Failed to create %s (%s)\n", path,
				    AG_GetError());
			}
		}
		ES_SchemLibraryRegisterDir(path);
	}
#endif
	if (ES_SchemLibraryLoad() == -1)
		AG_Verbose("Loading library: %s", AG_GetError());
}

/* Destroy the schematic library. */
void
ES_SchemLibraryDestroy(void)
{
	AG_ObjectDestroy(esSchemLibrary);
	esSchemLibrary = NULL;
	Free(esSchemLibraryDirs);
}

static AG_TlistItem *
FindSchems(AG_Tlist *tl, AG_Object *pob, int depth)
{
	AG_Object *chld;
	AG_TlistItem *it;

	it = AG_TlistAddPtr(tl,
	    AG_OfClass(pob, "ES_Schem:*") ?
	    esIconComponent.s : agIconDirectory.s,
	    pob->name, pob);
	it->depth = depth;

	if (!TAILQ_EMPTY(&pob->children)) {
		it->flags |= AG_TLIST_HAS_CHILDREN;
	}
	if ((it->flags & AG_TLIST_HAS_CHILDREN) &&
	    AG_TlistVisibleChildren(tl, it)) {
		TAILQ_FOREACH(chld, &pob->children, cobjs)
			FindSchems(tl, chld, depth+1);
	}
	return (it);
}

/* Generate a Tlist tree for the schem library. */
static void
PollLibrary(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	AG_TlistItem *ti;

	AG_TlistClear(tl);
	AG_LockVFS(esSchemLibrary);

	ti = FindSchems(tl, OBJECT(esSchemLibrary), 0);
	ti->flags |= AG_TLIST_EXPANDED;

	AG_UnlockVFS(esSchemLibrary);
	AG_TlistRestore(tl);
}

static void
InsertSchematic(AG_Event *event)
{
	VG_View *vv = AG_PTR(1);
	AG_TlistItem *ti = AG_PTR(2);
	ES_Schem *schem = ti->p1;
	
	if (!AG_OfClass(schem, "ES_Schem:*"))
		return;

	AG_TextMsg(AG_MSG_ERROR, "Insert schem %s in %s", OBJECT(schem)->name,
	    OBJECT(vv)->name);
}

static void
RefreshLibrary(AG_Event *event)
{
	if (ES_SchemLibraryLoad() == -1) {
		AG_TextMsgFromError();
		return;
	}
}

static void
EditSchem(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);

	(void)ES_OpenObject(obj);
}

static void
SaveSchem(AG_Event *event)
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
		    _("Successfully saved schematic to %s"), sname);
	} else {
		sname = AG_ShortFilename(path);
		if (AG_ObjectSaveToFile(obj, path) == -1) {
			AG_TextMsgFromError();
			return;
		}
		AG_ObjectSetArchivePath(obj, path);
		AG_ObjectSetNameS(obj, sname);
		AG_TextTmsg(AG_MSG_INFO, 1250,
		    _("Successfully saved schematic to %s"),
		    sname);
	}
}

static void
SaveSchemAsDlg(AG_Event *event)
{
	AG_Object *obj = AG_PTR(1);
	AG_Window *win;
	AG_FileDlg *fd;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("Save %s as..."), obj->name);

	fd = AG_FileDlgNewMRU(win, "edacious.mru.schems",
	    AG_FILEDLG_SAVE|AG_FILEDLG_CLOSEWIN|AG_FILEDLG_EXPAND);
	AG_FileDlgSetOptionContainer(fd, AG_BoxNewVert(win, AG_BOX_HFILL));
	AG_FileDlgAddType(fd, _("Edacious schematic"), "*.esh",
	    SaveSchem, "%p", obj);
	AG_WindowShow(win);
}

static void
SchemMenu(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	AG_Object *obj = AG_TlistSelectedItemPtr(tl);
	AG_PopupMenu *pm;

	if (!AG_OfClass(obj, "ES_Schem:*"))
		return;
	
	pm = AG_PopupNew(tl);

	AG_MenuAction(pm->item, _("Edit schematic..."), esIconCircuit.s,
	    EditSchem, "%p", obj);
	AG_MenuSeparator(pm->item);
	AG_MenuAction(pm->item, _("Save"), agIconSave.s,
	    SaveSchem, "%p,%s", obj, "");
	AG_MenuAction(pm->item, _("Save as..."), agIconSave.s,
	    SaveSchemAsDlg, "%p", obj);
	
	AG_PopupShowAt(pm,
	    WIDGET(tl)->drv->mouse->x,
	    WIDGET(tl)->drv->mouse->y);
}

ES_SchemLibraryEditor *
ES_SchemLibraryEditorNew(void *parent, VG_View *vv, Uint flags)
{
	AG_Box *box;
	AG_Button *btn;
	AG_Tlist *tl;

	box = AG_BoxNewVert(parent, AG_BOX_EXPAND);

	tl = AG_TlistNewPolled(box, AG_TLIST_TREE|AG_TLIST_EXPAND,
	    PollLibrary, NULL);
	AG_WidgetSetFocusable(tl, 0);

	AG_TlistSizeHint(tl, "XXXXXXXXXXXXXXXXXXX", 10);
	AG_TlistSetPopupFn(tl, SchemMenu, NULL);
	AG_SetEvent(tl, "tlist-dblclick", InsertSchematic, "%p", vv);

	btn = AG_ButtonNewFn(box, AG_BUTTON_HFILL, _("Refresh list"),
	    RefreshLibrary, NULL);
	AG_WidgetSetFocusable(btn, 0);

	RefreshLibrary(NULL);
	return (ES_SchemLibraryEditor *)box;
}
