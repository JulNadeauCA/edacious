/*	$Csoft$	*/

/*
 * Copyright (c) 2006 CubeSoft Communications, Inc.
 * <http://www.csoft.org>
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

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>

#include <agar/sc/sc_plotter.h>

#include "eda.h"
#include "scope.h"

const AG_Version esScopeVer = {
	"agar-eda scope",
	0, 0
};

const AG_ObjectOps esScopeOps = {
	ES_ScopeInit,
	ES_ScopeReinit,
	ES_ScopeDestroy,
	ES_ScopeLoad,
	ES_ScopeSave,
	ES_ScopeEdit
};

ES_Scope *
ES_ScopeNew(void *parent, const char *name)
{
	ES_Scope *scope;

	scope = Malloc(sizeof(ES_Scope), M_OBJECT);
	ES_ScopeInit(scope, name);
	AG_ObjectAttach(parent, scope);
	return (scope);
}

static void
ES_ScopeAttached(AG_Event *event)
{
	ES_Circuit *ckt = AG_SENDER();
	ES_Scope *scope = AG_SELF();

	if (!AGOBJECT_SUBCLASS(ckt, "circuit")) {
		AG_FatalError("bad parent");
	}
	scope->ckt = ckt;
}

void
ES_ScopeInit(void *obj, const char *name)
{
	ES_Scope *scope = obj;

	AG_ObjectInit(scope, "scope", name, &esScopeOps);
	scope->ckt = NULL;
	AG_SetEvent(scope, "attached", ES_ScopeAttached, NULL);
}

void
ES_ScopeReinit(void *obj)
{
	ES_Scope *scope = obj;
}

void
ES_ScopeDestroy(void *obj)
{
	ES_Scope *scope = obj;
}

int
ES_ScopeLoad(void *obj, AG_Netbuf *buf)
{
	ES_Scope *scope = obj;

	if (AG_ReadVersion(buf, &esScopeVer, NULL) != 0)
		return (-1);

	return (0);
}

int
ES_ScopeSave(void *obj, AG_Netbuf *buf)
{
	ES_Scope *scope = obj;

	AG_WriteVersion(buf, &esScopeVer);
	return (0);
}

static void
PollCktValues(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	AG_Prop *prop;
	AG_TlistItem *it;

	AG_TlistClear(tl);
	TAILQ_FOREACH(prop, &AGOBJECT(ckt)->props, props) {
		char pval[32];

		AG_PropPrint(pval, sizeof(pval), ckt, prop->key);
		it = AG_TlistAdd(tl, NULL, "%s (%s)", prop->key, pval);
		it->p1 = prop;
	}
	AG_TlistRestore(tl);
}

static void
AddCktPlot(AG_Event *event)
{
	char prop_path[AG_PROP_PATH_MAX];
	ES_Circuit *ckt = AG_PTR(1);
	SC_Plotter *ptr = AG_PTR(2);
	AG_TlistItem *it = AG_PTR(3);
	AG_Prop *prop = it->p1;
	SC_Plot *pl;

	AG_PropPath(prop_path, sizeof(prop_path), ckt, prop->key);
	pl = SC_PlotFromProp(ptr, SC_PLOT_LINEAR, prop->key, prop_path);
	SC_PlotSetXoffs(pl, ptr->xMax-1);
	SC_PlotSetScale(pl, 0.0, 15.0);
}

static void
UpdateScope(AG_Event *event)
{
	SC_Plotter *ptr = AG_PTR(1);

	SC_PlotterUpdate(ptr);
}


void *
ES_ScopeEdit(void *obj)
{
	ES_Scope *scope = obj;
	ES_Circuit *ckt = scope->ckt;
	AG_Window *win;
	SC_Plotter *ptr;
	AG_Combo *com;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, AGOBJECT(scope)->name);
	AG_WindowSetPosition(win, AG_WINDOW_UPPER_RIGHT, 0);

	ptr = SC_PlotterNew(win, SC_PLOTTER_EXPAND);
	com = AG_ComboNew(win, AG_COMBO_HFILL|AG_COMBO_POLL, _("Add plot: "));
	AG_SetEvent(com->list, "tlist-poll", PollCktValues, "%p", ckt);
	AG_SetEvent(com, "combo-selected", AddCktPlot, "%p,%p", ckt, ptr);
	AG_SetEvent(scope, "circuit-step-end", UpdateScope, "%p", ptr);

	AG_WindowScale(win, -1, -1);
	AG_WindowSetGeometry(win, agView->w/2, 0, agView->w/2, agView->h/3);
	return (win);
}

