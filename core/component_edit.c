/*
 * Copyright (c) 2008-2015 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Component edition interface.
 */

#include "core.h"

#define DEFAULT_SCHEM_SCALE	4
#define DEFAULT_CIRCUIT_SCALE	0

/*
 * Generate Tlist tree for Component subclasses.
 */
static AG_TlistItem *
FindClasses(AG_Tlist *tl, AG_ObjectClass *cl, int depth)
{
	ES_ComponentClass *clCom = (ES_ComponentClass *)cl;
	AG_ObjectClass *clSub;
	AG_TlistItem *it = NULL;

	if (!AG_ClassIsNamed(cl, "ES_Circuit:ES_Component:*")) {
		depth--;
		goto recurse;
	}
	it = AG_TlistAddS(tl,
	    (clCom->icon != NULL) ? clCom->icon->s : NULL,
	    clCom->name);
	it->depth = depth;
	it->p1 = cl;

	if (AG_ClassIsNamed(cl, "ES_Circuit:ES_Component")) {
		it->flags |= AG_TLIST_ITEM_EXPANDED;
	}
	if (!TAILQ_EMPTY(&cl->pvt.sub)) {
		it->flags |= AG_TLIST_HAS_CHILDREN;
	}
	if ((it->flags & AG_TLIST_HAS_CHILDREN) &&
	    AG_TlistVisibleChildren(tl, it)) {
		goto recurse;
	}
	return (it);
recurse:
	TAILQ_FOREACH(clSub, &cl->pvt.sub, pvt.subclasses) {
		(void)FindClasses(tl, clSub, depth+1);
	}
	return (it);
}
void
ES_ComponentListClasses(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();

	AG_TlistClear(tl);
	FindClasses(tl, &agObjectClass, 0);
	AG_TlistRestore(tl);
}

static void
Delete(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_View *vv = AG_PTR(2);
	ES_Circuit *ckt = com->ckt;
	
	VG_ClearEditAreas(vv);
	ES_LockCircuit(ckt);
	
	if (!AG_ObjectInUse(com)) {
		VG_Status(vv, _("Removed component %s."), OBJECT(com)->name);
		AG_ObjectDelete(com);
		ES_CircuitModified(ckt);
	} else {
		VG_Status(vv, _("Cannot delete %s: Component is in use!"),
		    OBJECT(com)->name);
	}
	ES_UnlockCircuit(ckt);
}

static void
Rotate(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	float theta = AG_FLOAT(2);
	VG_Node *vn;
	
	TAILQ_FOREACH(vn, &com->schemEnts, user)
		VG_Rotate(vn, theta);
}

static void
FlipHoriz(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_Node *vn;
	
	TAILQ_FOREACH(vn, &com->schemEnts, user)
		VG_FlipHoriz(vn);
}

static void
FlipVert(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_Node *vn;
	
	TAILQ_FOREACH(vn, &com->schemEnts, user)
		VG_FlipVert(vn);
}

static void
SuppressComponent(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_View *vv = AG_PTR(2);
	ES_Circuit *ckt = com->ckt;
	
	ES_LockCircuit(ckt);
	AG_PostEvent(ckt, com, "circuit-disconnected", NULL);
	com->flags |= ES_COMPONENT_SUPPRESSED;
	VG_Status(vv, _("Suppressed component %s."), OBJECT(com)->name);
	ES_UnlockCircuit(ckt);
}

static void
UnsuppressComponent(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_View *vv = AG_PTR(2);
	ES_Circuit *ckt = com->ckt;
	
	ES_LockCircuit(ckt);
	VG_Status(vv, _("Unsuppressed component %s."), OBJECT(com)->name);
	com->flags &= ~(ES_COMPONENT_SUPPRESSED);
	AG_PostEvent(ckt, com, "circuit-connected", NULL);
	ES_UnlockCircuit(ckt);
}

static void
DeleteSelections(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	VG_View *vv = AG_PTR(2);
	ES_Component *com;
	int changed = 0;

	VG_ClearEditAreas(vv);
	ES_LockCircuit(ckt);
rescan:
	CIRCUIT_FOREACH_COMPONENT_SELECTED(com, ckt) {
		if (!AG_ObjectInUse(com)) {
			AG_ObjectDelete(com);
			changed++;
		} else {
			VG_Status(vv,
			    _("Cannot delete %s: Component is in use!"),
			    OBJECT(com)->name);
		}
		goto rescan;
	}
	if (changed) {
		ES_CircuitModified(ckt);
	}
	ES_UnlockCircuit(ckt);
}

#if 0
static void
LoadComponent(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);

	if (AG_ObjectLoad(com) == -1)
		AG_TextMsgFromError();
}

static void
SaveComponent(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);

	if (AG_ObjectSave(com) == -1)
		AG_TextMsgFromError();
}
#endif

static void
SelectCircuitTool(AG_Event *event)
{
	VG_View *vv = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	VG_ToolOps *ops = AG_PTR(3);
	VG_Tool *t;

	if ((t = VG_ViewFindToolByOps(vv, ops)) != NULL) {
		VG_ViewSelectTool(vv, t, ckt);
	} else {
		AG_TextMsg(AG_MSG_ERROR, _("No such tool: %s"), ops->name);
	}
}

static void
PollPorts(AG_Event *event)
{
	char text[AG_TLIST_LABEL_MAX];
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	ES_Component *com;
	ES_Port *port;
	int i;
	
	AG_TlistClear(tl);
	
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->flags & ES_COMPONENT_SELECTED)
			break;
	}
	if (com == NULL)
		goto out;

	AG_ObjectLock(com);
	COMPONENT_FOREACH_PORT(port, i, com) {
		Snprintf(text, sizeof(text), "%d (%s) -> n%d [%.03fV]",
		    port->n, port->name, port->node,
		    ES_NodeVoltage(ckt, port->node));
		AG_TlistAddPtr(tl, NULL, text, port);
	}
	AG_ObjectUnlock(com);
out:
	AG_TlistRestore(tl);
}

static void
PortInfo(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	AG_Window *win;
	AG_Tlist *tl;

	if ((win = AG_WindowNewNamed(0, "%s-port-info", OBJECT(com)->name))
	    == NULL) {
		return;
	}
	tl = AG_TlistNew(win, AG_TLIST_POLL|AG_TLIST_EXPAND);
	AG_TlistSizeHint(tl, "1 (PORT) -> N123 [0.000V]", 4);
	AG_SetEvent(tl, "tlist-poll", PollPorts, "%p", com->ckt);
	AG_ButtonNewFn(win, AG_BUTTON_HFILL, _("Close"), AGWINCLOSE(win));
	AG_WindowShow(win);
}

/* Generate a popup menu for the given Component. */
void
ES_ComponentMenu(ES_Component *com, VG_View *vv)
{
	AG_PopupMenu *pm;
	AG_MenuItem *mRoot, *mi;
	Uint nsel = 0;
	ES_Component *com2;
	int common_class = 1;
	ES_ComponentClass *clCom = NULL;

	CIRCUIT_FOREACH_COMPONENT_SELECTED(com2, com->ckt) {
		if (clCom != NULL && clCom != COMCLASS(com2)) {
			common_class = 0;
		}
		clCom = COMCLASS(com2);
		nsel++;
	}

	if ((pm = AG_PopupNew(vv)) == NULL) {
		return;
	}
	mRoot = pm->root;
	{
		extern VG_ToolOps esWireTool;

		AG_MenuState(mRoot, (vv->curtool == NULL) ||
		                    (vv->curtool->ops != &esSchemSelectTool));
		AG_MenuAction(mRoot, _("Select tool"), esIconSelectArrow.s,
		    SelectCircuitTool, "%p,%p,%p", vv, com->ckt,
		    &esSchemSelectTool);

		AG_MenuState(mRoot, (vv->curtool == NULL) ||
		                    (vv->curtool->ops != &esWireTool));
		AG_MenuAction(mRoot, _("Wire tool"), esIconInsertWire.s,
		    SelectCircuitTool, "%p,%p,%p", vv, com->ckt,
		    &esWireTool);

		AG_MenuState(mRoot, 1);
		AG_MenuSeparator(mRoot);
	}

	AG_MenuSection(mRoot, _("[Component: %s]"), OBJECT(com)->name);

	mi = AG_MenuNode(mRoot, _("Transform"), esIconRotate.s);
	{
		AG_MenuAction(mi, _("Rotate 90\xc2\xb0 CW"),  esIconRotate.s, Rotate, "%p,%f", com, M_PI/2.0f);
		AG_MenuAction(mi, _("Rotate 90\xc2\xb0 CCW"), esIconRotate.s, Rotate, "%p,%f", com, -M_PI/2.0f);
		AG_MenuSeparator(mi);
		AG_MenuAction(mi, _("Rotate 45\xc2\xb0 CW"),  esIconRotate.s, Rotate, "%p,%f", com, M_PI/4.0f);
		AG_MenuAction(mi, _("Rotate 45\xc2\xb0 CCW"), esIconRotate.s, Rotate, "%p,%f", com, -M_PI/4.0f);
		AG_MenuSeparator(mi);
		AG_MenuAction(mi, _("Flip horizontally"), esIconFlipHoriz.s, FlipHoriz, "%p", com);
		AG_MenuAction(mi, _("Flip vertically"), esIconFlipVert.s,    FlipVert, "%p", com);
	}
	
	AG_MenuAction(mRoot, _("Delete"), agIconTrash.s,
	    Delete, "%p,%p", com, vv);
	if (com->flags & ES_COMPONENT_SUPPRESSED) {
		AG_MenuState(mRoot, 1);
		AG_MenuAction(mRoot, _("Unsuppress"), esIconStartSim.s,
		    UnsuppressComponent, "%p,%p", com, vv);
		AG_MenuState(mRoot, 0);
	} else {
		AG_MenuAction(mRoot, _("Suppress"), esIconStopSim.s,
		    SuppressComponent, "%p,%p", com, vv);
	}

	AG_MenuSeparator(mRoot);

	AG_MenuAction(mRoot, _("Port information..."), esIconPortEditor.s,
	    PortInfo, "%p,%p", com, vv);

	if (COMCLASS(com)->instance_menu != NULL) {
		AG_MenuSeparator(mRoot);
		COMCLASS(com)->instance_menu(com, mRoot);
	}
	if (nsel > 1) {
		if (common_class && COMCLASS(com)->class_menu != NULL) {
			AG_MenuSeparator(mRoot);
			AG_MenuSection(mRoot, _("[Class: %s]"), clCom->name);
			COMCLASS(com)->class_menu(com->ckt, mRoot);
		}
		AG_MenuSeparator(mRoot);
		AG_MenuSectionS(mRoot, _("[All selections]"));
		AG_MenuAction(mRoot, _("Delete selected"), agIconTrash.s,
		    DeleteSelections, "%p, %p", com->ckt, vv);
	}

	AG_PopupShow(pm);
}

/* Create an extra VG view tied to a given parent window. */
static void
CreateAlternateVGView(AG_Event *event)
{
	AG_Window *winParent = AG_PTR(1);
	AG_Object *obj = AG_PTR(2);
	VG_View *vvOrig = AG_PTR(3), *vvNew;
	AG_Window *win;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, _("View of %s"), obj->name);

	vvNew = VG_ViewNew(win, vvOrig->vg, VG_VIEW_EXPAND);
	AG_WidgetFocus(vvNew);
	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_TR, 50, 30);
	AG_WindowAttach(winParent, win);
	AG_WindowShow(win);
}

/* Update the equivalent circuit component list. */
static void
FindObjects(AG_Tlist *tl, AG_Object *pob, int depth, void *ckt)
{
	AG_Object *cob;
	AG_TlistItem *it;

	if (AG_OfClass(pob, "ES_Circuit:ES_Component:*")) {
		if (ESCOMPONENT(pob)->flags & ES_COMPONENT_SUPPRESSED) {
			it = AG_TlistAdd(tl, esIconComponent.s,
			    _("%s (suppressed)"),
			    pob->name);
		} else {
			it = AG_TlistAddS(tl, esIconComponent.s, pob->name);
		}
		it->selected = (ESCOMPONENT(pob)->flags &
		                ES_COMPONENT_SELECTED);
	} else {
		it = AG_TlistAddS(tl, NULL, pob->name);
	}
	it->depth = depth;
	it->cat = "object";
	it->p1 = pob;
	if (!TAILQ_EMPTY(&pob->children)) {
		it->flags |= AG_TLIST_HAS_CHILDREN;
		if (pob == OBJECT(ckt))
			it->flags |= AG_TLIST_ITEM_EXPANDED;
	}
	if ((it->flags & AG_TLIST_HAS_CHILDREN) &&
	    AG_TlistVisibleChildren(tl, it)) {
		TAILQ_FOREACH(cob, &pob->children, cobjs)
			FindObjects(tl, cob, depth+1, ckt);
	}
}
static void
PollObjects(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	AG_Object *ckt = AG_PTR(1);

	AG_TlistClear(tl);
	AG_LockVFS(ckt);
	FindObjects(tl, ckt, 0, ckt);
	AG_UnlockVFS(ckt);
	AG_TlistRestore(tl);
}

/* User has selected an equivalent circuit component. */
static void
SelectedObject(AG_Event *event)
{
	VG_View *vv = AG_PTR(1);
	AG_TlistItem *it = AG_PTR(2);
	int state = AG_INT(3);
	AG_Object *obj = it->p1;

	if (AG_OfClass(obj, "ES_Circuit:ES_Component:*")) {
		if (state) {
			ES_SelectComponent(COMPONENT(obj), vv);
		} else {
			ES_UnselectComponent(COMPONENT(obj), vv);
		}
	}
}

/* Click on equivalent circuit view */
static void
CircuitMouseButtonDown(AG_Event *event)
{
	VG_View *vv =  AG_SELF();
	int button = AG_INT(1);
	float x = AG_FLOAT(2);
	float y = AG_FLOAT(3);
	VG_Node *vn;

	if (button != AG_MOUSE_RIGHT) {
		return;
	}
	if ((vn = ES_SchemNearest(vv, VGVECTOR(x,y))) != NULL) {
		if (VG_NodeIsClass(vn, "SchemBlock")) {
			ES_SchemBlock *sb = (ES_SchemBlock *)vn;
			ES_SelectComponent(sb->com, vv);
			ES_ComponentMenu(sb->com, vv);
		} else if (VG_NodeIsClass(vn, "SchemWire")) {
			ES_SchemWire *sw = (ES_SchemWire *)vn;
			ES_SelectComponent(sw->wire, vv);
			ES_ComponentMenu(sw->wire, vv);
		}
	}
}

/* User has selected a schematic from the list. */
static void
SelectSchem(AG_Event *event)
{
	VG_View *vv = AG_PTR(1);
	AG_TlistItem *ti = AG_PTR(2);
	ES_Schem *scm = ti->p1;
	
	VG_ViewSetVG(vv, scm->vg);
	VG_ViewSetScale(vv, DEFAULT_SCHEM_SCALE);
	VG_Status(vv, _("Selected: %s"), ti->text);
}

/* Update schematics list. */
static void
PollSchems(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Component *com = AG_PTR(1);
	VG_View *vv = AG_PTR(2);
	AG_TlistItem *ti = NULL;
	ES_Schem *scm;
	int i = 0;

	AG_ObjectLock(vv);
	AG_TlistBegin(tl);
	TAILQ_FOREACH(scm, &com->schems, schems) {
		ti = AG_TlistAdd(tl, vgIconDrawing.s,
		    (vv->vg == scm->vg) ?
		    _("Schematic #%d [*]") :
		    _("Schematic #%d"),
		    i++);
		ti->p1 = scm;
	}
	AG_TlistEnd(tl);
	AG_ObjectUnlock(vv);
}

/* "New schematic" button pressed. */
static void
NewSchem(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_View *vv = AG_PTR(2);
	ES_Schem *scm;

	scm = ES_SchemNew(NULL);
	TAILQ_INSERT_TAIL(&com->schems, scm, schems);
	VG_ViewSetVG(vv, scm->vg);
	VG_ViewSetScale(vv, DEFAULT_SCHEM_SCALE);
}

/* "Delete schematic" button pressed. */
static void
DeleteSchem(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	AG_TlistItem *it = AG_TLIST_ITEM_PTR(2);
	VG_View *vv = AG_PTR(3);
	ES_Schem *scm;

	if ((scm = it->p1) == NULL)
		return;

	AG_ObjectLock(vv);
	if (vv->vg == scm->vg) {
		VG_ViewSetVG(vv, NULL);
	}
	AG_ObjectUnlock(vv);
	
	TAILQ_REMOVE(&com->schems, scm, schems);
	AG_ObjectDestroy(scm);
}

/* Import a schematic from file. */
static void
ImportSchem(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_View *vv = AG_PTR(2);
	char *path = AG_STRING(3);
	ES_Schem *scm;

	if ((scm = AG_ObjectNew(&esVfsRoot, NULL, &esSchemClass)) == NULL) {
		goto fail;
	}
	if (AG_ObjectLoadFromFile(scm, path) == -1) {
		AG_ObjectDetach(scm);
		AG_ObjectDestroy(scm);
		goto fail;
	}
	AG_SetString(scm, "archive-path", path);
	AG_ObjectSetNameS(scm, AG_ShortFilename(path));

	TAILQ_INSERT_TAIL(&com->schems, scm, schems);

	VG_ViewSetVG(vv, scm->vg);
	VG_ViewSetScale(vv, DEFAULT_SCHEM_SCALE);
	VG_Status(vv, _("Imported: %s"), OBJECT(scm)->name);
	return;
fail:
	AG_TextMsgFromError();
}

/* "Import schematic" button pressed. */
static void
ImportSchemDlg(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	VG_View *vv = AG_PTR(2);
	AG_Window *win;
	AG_FileDlg *fd;

	win = AG_WindowNew(0);
	AG_WindowSetCaptionS(win, _("Import schematic..."));
	
	fd = AG_FileDlgNewMRU(win, "edacious.mru.schems",
	    AG_FILEDLG_LOAD|AG_FILEDLG_CLOSEWIN|AG_FILEDLG_EXPAND);
	AG_FileDlgAddType(fd, _("Edacious schematic"), "*.esh",
	    ImportSchem, "%p,%p", com, vv);
	AG_WindowShow(win);
}

#if 0
/* Evaluate state of "remove schematic" button. */
static int
EvalRemoveButtonState(AG_Event *event)
{
	AG_Tlist *tl = AG_PTR(1);
	AG_TlistItem *ti;

	return ((ti = AG_TlistSelectedItem(tl)) != NULL && ti->p1 != NULL);
}
#endif

/* Generate the [View / Equivalent circuit] menu. */
static void
Menu_View_EquivalentCircuit(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	AG_Window *winParent = AG_PTR(2);
	VG_View *vv = AG_PTR(3);
	AG_MenuItem *mi = AG_PTR(4);
	AG_MenuItem *mSchem;

	AG_MenuActionKb(mi, _("New schematics view..."), esIconCircuit.s,
	    AG_KEY_V, AG_KEYMOD_CTRL,
	    CreateAlternateVGView, "%p,%p,%p", winParent, com, vv);

	AG_MenuSeparator(mi);

	mSchem = AG_MenuNode(mi, _("Schematic"), esIconCircuit.s);
	{
		AG_MenuFlags(mSchem, _("Nodes annotations"),
		    esIconShowNodes.s,
		    &ESCIRCUIT(com)->flags, ES_CIRCUIT_SHOW_NODES, 0);
		AG_MenuFlags(mSchem, _("Node names/numbers"),
		    esIconShowNodeNames.s,
		    &ESCIRCUIT(com)->flags, ES_CIRCUIT_SHOW_NODENAMES, 0);

		AG_MenuSeparator(mSchem);

		AG_MenuFlags(mSchem, _("Grid"), vgIconSnapGrid.s,
		    &vv->flags, VG_VIEW_GRID, 0);
		AG_MenuFlags(mSchem, _("Construction geometry"),
		    esIconConstructionGeometry.s,
		    &vv->flags, VG_VIEW_CONSTRUCTION, 0);
	}
}

/* Update the list of associated device packages. */
static void
PollPackages(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Component *com = AG_PTR(1);
	ES_ComponentPkg *pkg;
	AG_TlistItem *ti;

	AG_TlistBegin(tl);
	TAILQ_FOREACH(pkg, &com->pkgs, pkgs) {
		if (pkg->devName[0] != '\0') {
			ti = AG_TlistAdd(tl, esIconComponent.s, "%s (%s)",
			    pkg->name, pkg->devName);
		} else {
			ti = AG_TlistAddS(tl, esIconComponent.s, pkg->name);
		}
		ti->p1 = pkg;
	}
	AG_TlistEnd(tl);
}

/* Selected device package. */
static void
SelectPackage(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	AG_Table *tblPins = AG_PTR(2);
	AG_TlistItem *ti = AG_PTR(3);
	ES_ComponentPkg *pkg = (ti != NULL) ? ti->p1 : AG_PTR(4);
	AG_Numerical *num;
	Uint i;

	AG_TableBegin(tblPins);
	for (i = 0; i < com->nports; i++) {
		num = AG_NumericalNewInt(NULL, 0, NULL, NULL, &pkg->pins[i]);
		AG_TableAddRow(tblPins, "%i:%[W]",
		    com->ports[i+1].n, num);
	}
	AG_TableEnd(tblPins);
}

/* Link the component with a new device package. */
static void
AddPackage(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	AG_Textbox *tb = AG_PTR(2);
	AG_Textbox *tbDev = AG_PTR(3);
	AG_Table *tblPins = AG_PTR(4);
	ES_ComponentPkg *pkg;
	AG_Event ev;
	int i;

	pkg = Malloc(sizeof(ES_ComponentPkg));
	AG_TextboxCopyString(tb, pkg->name, sizeof(pkg->name));
	AG_TextboxCopyString(tbDev, pkg->devName, sizeof(pkg->devName));

	if (pkg->name[0] == '\0') {
		AG_TextError(_("Please enter a package name."));
		free(pkg);
		return;
	}

	pkg->pins = Malloc(com->nports*sizeof(int));
	pkg->flags = 0;
	TAILQ_INSERT_TAIL(&com->pkgs, pkg, pkgs);
	
	for (i = 0; i < com->nports; i++)
		pkg->pins[i] = 0;

	AG_TextboxClearString(tb);
	AG_TextboxClearString(tbDev);
	
	AG_EventArgs(&ev, "%p,%p,%p,%p", com, tblPins, NULL, pkg);
	SelectPackage(&ev);
}

/* Remove a device package. */
static void
RemovePackage(AG_Event *event)
{
	ES_Component *com = AG_PTR(1);
	ES_ComponentPkg *pkg = AG_PTR(2);

	TAILQ_REMOVE(&com->pkgs, pkg, pkgs);
	Free(pkg->pins);
	free(pkg);
}

static void
PackagePopup(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	AG_TlistItem *ti = AG_TlistSelectedItem(tl);
	ES_Component *com = AG_PTR(1);
	AG_PopupMenu *pm;

	if ((pm = AG_PopupNew(tl)) == NULL) {
		return;
	}
	AG_MenuAction(pm->root, _("Delete"), agIconTrash.s, RemovePackage, "%p,%p", com, ti->p1);
	AG_PopupShow(pm);
}

void *
ES_ComponentEdit(void *obj)
{
	ES_Component *com = obj;
	ES_Circuit *ckt = obj;
	AG_Pane *hPane;
	AG_Window *win;
	AG_Menu *menu;
	AG_MenuItem *m, *mView;
	AG_Notebook *nb;
	AG_NotebookTab *nt;
	VG_ToolOps **pOps, *ops;
	VG_Tool *tool;
	AG_Button *btn;
	AG_Pane *vPane;

	win = AG_WindowNew(0);
	menu = AG_MenuNew(win, AG_MENU_HFILL);
	m = AG_MenuNode(menu->root, _("File"), NULL);
	ES_FileMenu(m, com);
	m = AG_MenuNode(menu->root, _("Edit"), NULL);
	ES_EditMenu(m, com);
	mView = AG_MenuNode(menu->root, _("View"), NULL);
	
	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);

	/*
	 * Schematic block editor
	 */
	nt = AG_NotebookAdd(nb, _("Schematics"), AG_BOX_HORIZ);
	{
		VG_View *vv;
		AG_Tlist *tlSchems;
		AG_Box *bCmds;
		AG_Toolbar *tb;

		vv = VG_ViewNew(NULL, NULL, VG_VIEW_EXPAND|VG_VIEW_GRID|
		                            VG_VIEW_CONSTRUCTION);
		VG_ViewSetSnapMode(vv, VG_GRID);
		VG_ViewSetScale(vv, DEFAULT_SCHEM_SCALE);
		VG_ViewSetGrid(vv, 0, VG_GRID_POINTS, 2, VG_GetColorRGB(100,100,100));
		VG_ViewSetGrid(vv, 1, VG_GRID_POINTS, 8, VG_GetColorRGB(200,200,0));
	
		hPane = AG_PaneNewHoriz(nt, AG_PANE_EXPAND);

		/*
		 * Left side
		 */
		vPane = AG_PaneNewVert(hPane->div[0], AG_PANE_EXPAND);
		{
			/*
			 * Model schematics
			 */
			AG_LabelNew(vPane->div[0], 0, _("Schematic blocks:"));
			tlSchems = AG_TlistNewPolled(vPane->div[0], AG_TLIST_EXPAND,
			    PollSchems, "%p,%p", com, vv);
			AG_TlistSizeHint(tlSchems, "Schematic #0000", 5);
			AG_SetEvent(tlSchems, "tlist-dblclick",
			    SelectSchem, "%p", vv);
	
			if (!TAILQ_EMPTY(&com->schems)) {
				VG_ViewSetVG(vv, (TAILQ_FIRST(&com->schems))->vg);
				VG_ViewSetScale(vv, DEFAULT_SCHEM_SCALE);
			}

			bCmds = AG_BoxNewHorizNS(vPane->div[0], AG_BOX_HOMOGENOUS|AG_BOX_HFILL);
			{
				AG_ButtonNewFn(bCmds, 0, _("New"),
				    NewSchem, "%p,%p", com, vv);
				btn = AG_ButtonNewFn(bCmds, 0, _("Delete"),
				    DeleteSchem, "%p,%p,%p", com, tlSchems, vv);
#if 0
				AG_BindIntFn(btn, "state",
				    EvalRemoveButtonState, "%p", tlSchems);
#endif
			}
			bCmds = AG_BoxNewHorizNS(vPane->div[0], AG_BOX_HOMOGENOUS|AG_BOX_HFILL);
			{
				AG_ButtonNewFn(bCmds, 0, _("Import..."),
				    ImportSchemDlg, "%p,%p", com, vv);
			}
		}
		VG_AddEditArea(vv, vPane->div[1]);

		/*
		 * Right side
		 */
		{
			AG_ObjectAttach(hPane->div[1], vv);
		}
	
		tb = AG_ToolbarNew(nt, AG_TOOLBAR_VERT, 1, 0);
		{
			for (pOps = &esSchemTools[0]; *pOps != NULL; pOps++) {
				ops = *pOps;
				tool = VG_ViewRegTool(vv, ops, NULL);

				btn = AG_ToolbarButtonIcon(tb,
				    (ops->icon ? ops->icon->s : NULL), 0,
				    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool,
				    com);
				AG_BindIntMp(btn, "state", &tool->selected,
				    &OBJECT(vv)->pvt.lock);

				if (ops == &esSchemSelectTool) {
					VG_ViewSetDefaultTool(vv, tool);
					VG_ViewSelectTool(vv, tool, NULL);
				}
			}

			AG_ToolbarSeparator(tb);

			for (pOps = &esVgTools[0]; *pOps != NULL; pOps++) {
				ops = *pOps;
				if (ops == &vgSelectTool) {
					continue;	/* We use our own */
				}
				tool = VG_ViewRegTool(vv, ops, com);
				btn = AG_ToolbarButtonIcon(tb,
				    (ops->icon ? ops->icon->s : NULL), 0,
				    VG_ViewSelectToolEv, "%p,%p,%p",
				    vv, tool, com);
				AG_BindIntMp(btn, "state", &tool->selected,
				    &OBJECT(vv)->pvt.lock);
			}
		}
	}
	
	/*
	 * Equivalent circuit editor
	 */
	nt = AG_NotebookAdd(nb, _("Equivalent Circuit"), AG_BOX_VERT);
	hPane = AG_PaneNewHoriz(nt, AG_PANE_EXPAND);
	{
		VG_View *vv;
		AG_Notebook *nb;
		AG_NotebookTab *nt;
		ES_ComponentLibraryEditor *led;
		AG_Tlist *tl;
		AG_Box *hBox;
		AG_Toolbar *tb;
	
		vv = VG_ViewNew(NULL, ckt->vg, VG_VIEW_EXPAND|VG_VIEW_GRID);
		VG_ViewSetSnapMode(vv, VG_GRID);
		VG_ViewSetScale(vv, DEFAULT_CIRCUIT_SCALE);

		AG_MenuDynamicItem(mView, _("Equivalent circuit"),
		    esIconCircuit.s,
		    Menu_View_EquivalentCircuit, "%p,%p,%p", com, win, vv);

		vPane = AG_PaneNewVert(hPane->div[0], AG_PANE_EXPAND);
		nb = AG_NotebookNew(vPane->div[0], AG_NOTEBOOK_EXPAND);
		nt = AG_NotebookAdd(nb, _("Library"), AG_BOX_VERT);
		{
			led = ES_ComponentLibraryEditorNew(nt, vv, ckt, 0);
			AG_WidgetSetFocusable(led, 0);
		}
		nt = AG_NotebookAdd(nb, _("Objects"), AG_BOX_VERT);
		{
			tl = AG_TlistNewPolled(nt,
			    AG_TLIST_POLL | AG_TLIST_TREE | AG_TLIST_EXPAND |
			    AG_TLIST_NOSELSTATE,
			    PollObjects, "%p", ckt);

			AG_SetEvent(tl, "tlist-changed",
			    SelectedObject, "%p", vv);

			AG_WidgetSetFocusable(tl, 0);
		}
		VG_AddEditArea(vv, vPane->div[1]);

		hBox = AG_BoxNewHoriz(hPane->div[1], AG_BOX_EXPAND);
		AG_ObjectAttach(hBox, vv);
		AG_WidgetFocus(vv);
		tb = AG_ToolbarNew(hBox, AG_TOOLBAR_VERT, 1, 0);

		/* Register Circuit-specific tools */
		for (pOps = &esCircuitTools[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			tool = VG_ViewRegTool(vv, ops, ckt);

			btn = AG_ToolbarButtonIcon(tb,
			    (ops->icon ? ops->icon->s : NULL), 0,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, ckt);
			AG_BindIntMp(btn, "state", &tool->selected,
			    &OBJECT(vv)->pvt.lock);

			if (ops == &esSchemSelectTool) {
				VG_ViewSetDefaultTool(vv, tool);
				VG_ViewSelectTool(vv, tool, ckt);
			}
		}

		AG_ToolbarSeparator(tb);

		/* Register generic VG drawing tools */
		for (pOps = &esVgTools[0]; *pOps != NULL; pOps++) {
			ops = *pOps;
			if (ops == &vgSelectTool) {
				continue;	/* We use our own */
			}
			tool = VG_ViewRegTool(vv, ops, com);
			btn = AG_ToolbarButtonIcon(tb,
			    (ops->icon ? ops->icon->s : NULL), 0,
			    VG_ViewSelectToolEv, "%p,%p,%p", vv, tool, com);
			AG_BindIntMp(btn, "state", &tool->selected,
			    &OBJECT(vv)->pvt.lock);
		}
		
		/* Register (but hide) the special "insert component" tool. */
		VG_ViewRegTool(vv, &esComponentInsertTool, ckt);

		VG_ViewButtondownFn(vv, CircuitMouseButtonDown, NULL);
	}

	if (strcmp(OBJECT_CLASS(com)->hier, "ES_Circuit:ES_Component") != 0) {
		/*
		 * Model parameters editor
		 */
		nt = AG_NotebookAdd(nb, _("Model Parameters"), AG_BOX_VERT);
		{
			hPane = AG_PaneNewHoriz(nt, AG_PANE_EXPAND);
			if (OBJECT_CLASS(com)->edit != NULL) {
				AG_Widget *wEdit = OBJECT_CLASS(com)->edit(com);
				AG_ObjectAttach(hPane->div[0], wEdit);
			}
			AG_LabelNewS(hPane->div[1], 0, _("Notes:"));
			AG_TextboxNewS(hPane->div[1],
			    AG_TEXTBOX_EXPAND|AG_TEXTBOX_MULTILINE, NULL);
		}
	}
		
	/*
	 * Package list
	 */
	nt = AG_NotebookAdd(nb, _("Packages"), AG_BOX_VERT);
	hPane = AG_PaneNewHoriz(nt, AG_PANE_EXPAND);
	{
		AG_Tlist *tl;
		AG_Textbox *tb, *tbDev;
		AG_Box *vBox;
		AG_Table *tblPins;

		tblPins = AG_TableNew(hPane->div[1], AG_TABLE_EXPAND);
		AG_TableSetRowHeight(tblPins, agTextFontHeight+8);
		AG_TableAddCol(tblPins, _("Component Port#"), "< Component Port# >", NULL);
		AG_TableAddCol(tblPins, _("Package Pin#"), "< Package Pin# >", NULL);

		vBox = AG_BoxNewVert(hPane->div[0], AG_BOX_EXPAND);
		AG_LabelNew(vBox, 0, _("Available packages: "));
		tl = AG_TlistNewPolled(vBox, AG_TLIST_POLL|AG_TLIST_EXPAND,
		    PollPackages, "%p", com);
		AG_TlistSizeHint(tl, "<XXXXXXXXXXXXXXXXX>", 10);
		AG_TlistSetPopupFn(tl, PackagePopup, "%p", com);
		AG_TlistSetCompareFn(tl, AG_TlistCompareStrings);
		AG_TlistSetDblClickFn(tl, SelectPackage, "%p,%p", com, tblPins);

		tb = AG_TextboxNewS(vBox, AG_TEXTBOX_HFILL, _("Package: "));
		tbDev = AG_TextboxNewS(vBox, AG_TEXTBOX_HFILL, _("Device name: "));
		AG_ButtonNewFn(vBox, AG_BUTTON_HFILL, _("Add package"),
		    AddPackage, "%p,%p,%p,%p", com, tb, tbDev, tblPins);
	}

	AG_WindowSetGeometryAlignedPct(win, AG_WINDOW_MC, 50, 40);
	return (win);
}
