/*
 * Copyright (c) 2006-2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Oscilloscope tool. This is simply an interface to bind an M_Plotter(3)
 * to the user-specified Circuit values.
 */

#include "core.h"
#include <freesg/m/m_plotter.h>

ES_Scope *
ES_ScopeNew(void *parent, const char *name, ES_Circuit *ckt)
{
	ES_Scope *scope;

	scope = Malloc(sizeof(ES_Scope));
	AG_ObjectInit(scope, &esScopeClass);
	AG_ObjectSetName(scope, "%s", name);
	AG_ObjectAttach(parent, scope);

	ES_AddSimulationObj(ckt, scope);
	AG_ObjectAddDep(scope, ckt, 0);
	scope->ckt = ckt;
	return (scope);
}

static void
PostSimStep(AG_Event *event)
{
	ES_Scope *scope = AG_SELF();

	if (scope->plotter != NULL)
		M_PlotterUpdate(scope->plotter);
}

static void
Init(void *obj)
{
	ES_Scope *scope = obj;

	scope->ckt = NULL;
	scope->plotter = NULL;
	AG_SetEvent(scope, "circuit-step-end", PostSimStep, NULL);
}

static int
Load(void *obj, AG_DataSource *buf, const AG_Version *ver)
{
/*	ES_Scope *scope = obj; */
	return (0);
}

static int
Save(void *obj, AG_DataSource *buf)
{
	return (0);
}

static void
PollSrcs(AG_Event *event)
{
	char pval[32];
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	AG_Prop *prop;
	AG_TlistItem *it;

	AG_TlistClear(tl);
	TAILQ_FOREACH(prop, &OBJECT(ckt)->props, props) {
		AG_PropPrint(pval, sizeof(pval), ckt, prop->key);
		it = AG_TlistAdd(tl, NULL, "%s (%s)", prop->key, pval);
		it->p1 = prop;
	}
	AG_TlistRestore(tl);
}

static void
PollPlots(AG_Event *event)
{
	AG_Tlist *tl = AG_SELF();
	ES_Circuit *ckt = AG_PTR(1);
	M_Plotter *ptr = AG_PTR(2);
	M_Plot *pl;
	AG_TlistItem *it;

	AG_TlistClear(tl);
	TAILQ_FOREACH(pl, &ptr->plots, plots) {
		it = AG_TlistAdd(tl, NULL, "%s", pl->label_txt);
		it->p1 = pl;
		it->cat = "plot";
	}
	AG_TlistRestore(tl);
}

static void
AddPlotFromSrc(AG_Event *event)
{
	char prop_path[AG_PROP_PATH_MAX];
	ES_Circuit *ckt = AG_PTR(1);
	M_Plotter *ptr = AG_PTR(2);
	AG_Prop *prop = AG_PTR(3);
	M_Plot *pl;

	AG_PropCopyPath(prop_path, sizeof(prop_path), ckt, prop->key);
	pl = M_PlotFromProp(ptr, M_PLOT_LINEAR, prop->key, &esVfsRoot,
	    prop_path);
	M_PlotSetXoffs(pl, ptr->xMax-1);
	M_PlotSetScale(pl, 0.0, 15.0);
}

static void
AddPlotFromDerivative(AG_Event *event)
{
	char prop_path[AG_PROP_PATH_MAX];
	AG_Tlist *tl = AG_PTR(1);
	ES_Circuit *ckt = AG_PTR(2);
	M_Plotter *ptr = AG_PTR(3);
	AG_TlistItem *it = AG_TlistSelectedItem(tl);
	M_Plot *pl;

	pl = M_PlotFromDerivative(ptr, M_PLOT_LINEAR, (M_Plot *)it->p1);
	M_PlotSetXoffs(pl, ptr->xMax-1);
	M_PlotSetScale(pl, 0.0, 15.0);
}

static void
ShowPlotSettings(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	M_Plot *pl = AG_TLIST_ITEM(2);

	M_PlotSettings(pl);
}

static void *
Edit(void *obj)
{
	ES_Scope *scope = obj;
	ES_Circuit *ckt = scope->ckt;
	AG_Window *win;
	M_Plotter *ptr;
	AG_Tlist *tlSrcs, *tlPlots;
	AG_Pane *hPane;
	AG_Pane *vPane;

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, OBJECT(scope)->name);
	AG_WindowSetPosition(win, AG_WINDOW_UPPER_RIGHT, 0);

	hPane = AG_PaneNewHoriz(win, AG_PANE_EXPAND);
	{
		ptr = M_PlotterNew(hPane->div[1], M_PLOTTER_EXPAND);
		scope->plotter = ptr;

		vPane = AG_PaneNewVert(hPane->div[0],
		    AG_PANE_EXPAND|AG_PANE_DIV|AG_PANE_FORCE_DIV);
		{
			AG_Tlist *tl;
			AG_MenuItem *m;

			AG_LabelNewStatic(vPane->div[0], 0, _("Sources:"));
			tl = AG_TlistNew(vPane->div[0], AG_TLIST_EXPAND|
			                                AG_TLIST_POLL);
			AG_TlistSizeHint(tl, "XXXXXXXXXXXXX", 2);
			AG_SetEvent(tl, "tlist-poll", PollSrcs, "%p", ckt);
			AG_SetEvent(tl, "tlist-dblclick", AddPlotFromSrc,
			    "%p,%p", ckt, ptr);
	
			tl = AG_TlistNew(vPane->div[1], AG_TLIST_EXPAND|
			                                AG_TLIST_POLL);
			AG_SetEvent(tl, "tlist-poll", PollPlots,
			    "%p,%p", ckt, ptr);
			m = AG_TlistSetPopup(tl, "plot");
			AG_MenuAction(m, _("Plot derivative"), NULL,
			    AddPlotFromDerivative, "%p,%p,%p", tl, ckt, ptr);
			AG_MenuAction(m, _("Plot settings"), NULL,
			    ShowPlotSettings, "%p,%p", ckt, tl);
		}
	}
	
	AG_WindowSetGeometry(win, agView->w/2, 0, agView->w/2, agView->h/3);
	return (win);
}

AG_ObjectClass esScopeClass = {
	"ES_Scope",
	sizeof(ES_Scope),
	{ 0,0 },
	Init,
	NULL,			/* free_dataset */
	NULL,			/* destroy */
	Load,
	Save,
	Edit
};
