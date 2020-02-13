/*
 * Copyright (c) 2009-2012 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Layout editor: Tool for inserting a device package into the circuit layout.
 * Typically, it will be loaded from a file in the device package library.
 */

#include "core.h"

typedef struct es_package_insert_tool {
	VG_Tool _inherit;
	Uint flags;
#define INSERT_MULTIPLE	0x01		/* Insert multiple instances */
	ES_Package *floatingPkg;
} ES_PackageInsertTool;

#if 0
/*
 * Highlight the Package pin connections that would be made to existing
 * LayoutNodes.
 */
static void
HighlightConnections(VG_View *vv, ES_Layout *lo, ES_Component *com)
{
	char status[4096], s[128];
	ES_Port *port;
	ES_SchemPort *spNear;
	VG_Vector pos;
	int i;

	status[0] = '\0';
	COMPONENT_FOREACH_PORT(port, i, com) {
		if (port->sp == NULL) {
			continue;
		}
		pos = VG_Pos(port->sp);
		spNear = VG_PointProximityMax(vv, "SchemPort", &pos, NULL,
		    port->sp, vv->pointSelRadius);
		if (spNear != NULL) {
			ES_SelectPort(port);
			Snprintf(s, sizeof(s), "%d>[%s:%d] ",
			    i, OBJECT(spNear->com)->name, spNear->port->n);
		} else {
			ES_UnselectPort(port);
			Snprintf(s, sizeof(s), _("%d->(new) "), i);
		}
		Strlcat(status, s, sizeof(status));
	}
	VG_StatusS(vv, status);
}
#endif

/* Connect a floating package to the layout. */
static int
ConnectPackage(ES_PackageInsertTool *t, ES_Layout *lo, ES_Package *pkg)
{
//	VG_View *vv = VGTOOL(t)->vgv;

	/* Connect to the circuit. The package is no longer floating. */
	TAILQ_INSERT_TAIL(&lo->packages, pkg, packages);
	pkg->flags |= ES_PACKAGE_CONNECTED;
	
//	ES_UnselectAllComponents(lo, vv);
//	ES_SelectComponent(t->floatingCom, vv);
	t->floatingPkg = NULL;
	return (0);
}

/* Remove the current floating package if any. */
static void
RemoveFloatingPackage(ES_PackageInsertTool *t)
{
	if (t->floatingPkg != NULL) {
		AG_ObjectDetach(t->floatingPkg);
		AG_ObjectDestroy(t->floatingPkg);
		t->floatingPkg = NULL;
	}
}

static int
MouseButtonDown(void *p, VG_Vector vPos, int button)
{
	ES_PackageInsertTool *t = p;
	ES_Layout *lo = VGTOOL(t)->p;
	VG_View *vv = VGTOOL(t)->vgv;
	
	switch (button) {
	case AG_MOUSE_LEFT:
		if (t->floatingPkg != NULL) {
			if (ConnectPackage(t, lo, t->floatingPkg) == -1){
				AG_TextError("Connecting package: %s",
				    AG_GetError());
				break;
			}
			if (t->floatingPkg != NULL) {
				VG_Node *vn;

				TAILQ_FOREACH(vn, &t->floatingPkg->layoutEnts, user) {
					VG_SetPosition(vn, vPos);
				}
//				HighlightConnections(vv, lo, t->floatingPkg);
				return (1);
			}
		}
		break;
	case AG_MOUSE_MIDDLE:
		if (t->floatingPkg != NULL) {
			AG_KeyMod mod = AG_GetModState(vv);
			VG_Node *vn;

			TAILQ_FOREACH(vn, &t->floatingPkg->layoutEnts, user) {
				if (mod & AG_KEYMOD_CTRL) {
					VG_Rotate(vn, VG_PI/4.0f);
				} else {
					VG_Rotate(vn, VG_PI/2.0f);
				}
			}
		}
		break;
	case AG_MOUSE_RIGHT:
		RemoveFloatingPackage(t);
		VG_ViewSelectTool(vv, NULL, lo);
		return (1);
	default:
		return (0);
	}
	return (1);
}

/* Move a floating package and highlight the overlapping nodes. */
static int
MouseMotion(void *p, VG_Vector vPos, VG_Vector vRel, int b)
{
	ES_PackageInsertTool *t = p;
//	VG_View *vv = VGTOOL(t)->vgv;
//	ES_Layout *lo = VGTOOL(t)->p;
	VG_Node *vn;

	if (t->floatingPkg != NULL) {
		TAILQ_FOREACH(vn, &t->floatingPkg->layoutEnts, user) {
			VG_SetPosition(vn, vPos);
		}
//		HighlightConnections(vv, lo, t->floatingPkg);
		return (1);
	}
	return (0);
}

/* Insert a new floating package in the layout. */
static void
Insert(AG_Event *event)
{
	ES_PackageInsertTool *t = AG_PTR(1);
	ES_Layout *lo = ES_LAYOUT_PTR(2);
	ES_Package *pkgModel = ES_PACKAGE_PTR(3), *pkg;
	char name[AG_OBJECT_NAME_MAX];
	VG_View *vv = VGTOOL(t)->vgv;

	/* Allocate the Package instance and load from file. */
	if ((pkg = malloc(sizeof(ES_Package))) == NULL) {
		AG_TextMsgFromError();
		return;
	}
	AG_ObjectInit(pkg, &esPackageClass);
	if (!AG_Defined(pkgModel,"archive-path")) {
		AG_TextMsgFromError();
		return;
	}
	if (AG_ObjectLoadFromFile(pkg,
	    AG_GetStringP(pkgModel,"archive-path")) == -1) {
		AG_TextMsgFromError();
		AG_ObjectDestroy(pkg);
		return;
	}

	/* Generate a unique package name */
	AG_ObjectGenNamePfx(lo, "U", name, sizeof(name));
	AG_ObjectSetNameS(pkg, name);

	/* Attach to the circuit as a floating element. */
	t->floatingPkg = pkg;
	AG_ObjectAttach(lo, pkg);
//	ES_SelectComponent(pkg, vv);
	
	AG_WidgetFocus(vv);
}

/* Abort the current insert operation. */
static void
AbortInsert(AG_Event *event)
{
	ES_PackageInsertTool *t = AG_PTR(1);

	RemoveFloatingPackage(t);
	VG_ViewSelectTool(VGTOOL(t)->vgv, NULL, NULL);
}

static void
Init(void *p)
{
	ES_PackageInsertTool *t = p;
	VG_ToolCommand *tc;

	t->floatingPkg = NULL;
	t->flags = INSERT_MULTIPLE;

	VG_ToolCommandNew(t, "Insert", Insert);
	tc = VG_ToolCommandNew(t, "AbortInsert", AbortInsert);
	VG_ToolCommandKey(tc, AG_KEYMOD_NONE, AG_KEY_ESCAPE);
}

static void *
Edit(void *p, VG_View *vv)
{
	ES_PackageInsertTool *t = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	AG_CheckboxNewFlag(box, 0, _("Insert multiple instances"),
	    &t->flags, INSERT_MULTIPLE);
	return (box);
}

VG_ToolOps esPackageInsertTool = {
	N_("Insert package"),
	N_("Insert new device package into the layout."),
	NULL,
	sizeof(ES_PackageInsertTool),
	0,
	Init,
	NULL,			/* destroy */
	Edit,
	NULL,			/* predraw */
	NULL,			/* postdraw */
	NULL,			/* selected */
	NULL,			/* deselected */
	MouseMotion,
	MouseButtonDown,
	NULL,			/* mousebuttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};
