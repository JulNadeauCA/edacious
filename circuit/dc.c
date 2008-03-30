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
 * DC simulation based on the Modified Nodal Analysis (MNA). This involves:
 *
 *	- Linear DC analysis to evaluate the DC currents/voltages for
 *	  a lumped, linear circuit.
 *	- Nonlinear DC analysis to obtain the quiescent operating point
 *	  of a circuit containing nonlinear elements.
 */

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/sc.h>

#include "eda.h"
#include "sim.h"

/* Formulate the KCL/branch equations and compute A=LU. */
static int
FactorizeMNA(ES_SimDC *sim, ES_Circuit *ckt)
{
	ES_Component *com;
	u_int i, n, m;
	SC_Real d;

	SC_MatrixSetZero(sim->G);
	SC_MatrixSetZero(sim->B);
	SC_MatrixSetZero(sim->C);
	SC_MatrixSetZero(sim->D);
	SC_MatrixSetZero(sim->i);
	SC_MatrixSetZero(sim->e);

	/* Formulate the general equations. */
	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		if (com->loadDC_G != NULL)
			com->loadDC_G(com, sim->G);
		if (com->loadDC_BCD != NULL)
			com->loadDC_BCD(com, sim->B, sim->C, sim->D);
		if (com->loadDC_RHS != NULL)
			com->loadDC_RHS(com, sim->i, sim->e);
#if 0
		if (com->loadAC != NULL)
			com->loadAC(com, sim->Y);
		if (com->loadSP != NULL)
			com->loadSP(com, sim->S, sim->N);
#endif
	}

	/* Compose the right-hand side vector (i, e). */
	SC_MatrixCompose21(sim->z, sim->i, sim->e);

	/* Compose the coefficient matrix A from [G,B;C,D] and compute LU. */
	SC_MatrixCompose22(sim->A, sim->G, sim->B, sim->C, sim->D);
	return (((SC_FactorizeLU(sim->A, sim->LU, sim->piv, &d)) != NULL) ?
	        0 : -1);
}

static int
SolveMNA(ES_SimDC *sim, ES_Circuit *ckt)
{
	SC_VectorCopy(sim->z, sim->x);
	SC_BacksubstLU(sim->LU, sim->piv, sim->x);
	return (0);
}

static Uint32
StepMNA(void *obj, Uint32 ival, void *arg)
{
	ES_Circuit *ckt = obj;
	ES_SimDC *sim = arg;
	ES_Component *com;
	static Uint32 t1 = 0;
	SC_Matrix Aprev;
	Uint i = 1;

	if (ckt->m == 0) {
		AG_SetError(_("Circuit has no voltage sources"));
		goto halt;
	}
	if (ckt->n == 0) {
		AG_SetError(_("Circuit has no nodes to evaluate"));
		goto halt;
	}

	AG_PostEvent(NULL, ckt, "circuit-step-begin", NULL);

	/* Invoke the time step function of all models. */
	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		if (com->intStep != NULL)
			com->intStep(com, sim->Telapsed);
	}
solve:
	/* Find the initial DC operating point. */
	if (FactorizeMNA(sim, ckt) == -1 ||
	    SolveMNA(sim, ckt) == -1) {
		goto halt;
	}
	/*
	 * Save the A matrix and execute per-component procedural update
	 * functions. The intUpdate function can modify either A or the
	 * RHS in response to calculated node voltages.
	 */
	SC_MatrixAlloc(&Aprev, sim->A->m, sim->A->n);
	SC_MatrixCopy(sim->A, &Aprev);
	AGOBJECT_FOREACH_CLASS(com, ckt, es_component, "ES_Component:*") {
		if (com->intUpdate != NULL)
			com->intUpdate(com);
	}
	if (FactorizeMNA(sim, ckt) == -1 ||
	    SolveMNA(sim, ckt) == -1) {
		goto halt;
	}
	/* Loop until stability is reached. */
	/* TODO examine pattern changes to detect unstability. */
	if (SC_MatrixCompare(sim->A, &Aprev) != 0) {
		if (++i > sim->max_iters) {
			AG_SetError(_("Could not find stable solution in "
			              "%u iterations"), i);
			goto halt;
		}
		goto solve;
	}
	if (i > sim->iters_hiwat) { sim->iters_hiwat = i; }
	else if (i < sim->iters_lowat) { sim->iters_lowat = i; }

	/* TODO loop until stable */

	sim->TavgReal = SDL_GetTicks() - t1;
	t1 = SDL_GetTicks();
	sim->Telapsed++;
	AG_PostEvent(NULL, ckt, "circuit-step-end", NULL);
	return (ival);
halt:
	AG_TextMsg(AG_MSG_ERROR, _("%s; simulation stopped"), AG_GetError());
	SIM(sim)->running = 0;
	return (0);
}

static void
Init(void *p)
{
	ES_SimDC *sim = p;

	ES_SimInit(sim, &esSimDcOps);
	sim->Telapsed = 0;
	sim->TavgReal = 0;
	sim->speed = 500;
	sim->max_iters = 100000;
	sim->iters_hiwat = 1;
	sim->iters_lowat = 1;
	AG_SetTimeout(&sim->update_to, StepMNA, sim, AG_CANCEL_ONDETACH);

	sim->A = SC_MatrixNew(0, 0);
	sim->G = SC_MatrixNew(0, 0);
	sim->B = SC_MatrixNew(0, 0);
	sim->C = SC_MatrixNew(0, 0);
	sim->D = SC_MatrixNew(0, 0);
	sim->LU = SC_MatrixNew(0, 0);

	sim->z = SC_VectorNew(0);
	sim->i = SC_VectorNew(0);
	sim->e = SC_VectorNew(0);
	
	sim->x = SC_VectorNew(0);
	sim->v = SC_VectorNew(0);
	sim->j = SC_VectorNew(0);

	sim->piv = SC_IvectorNew(0);
}

static void
Resize(void *p, ES_Circuit *ckt)
{
	ES_SimDC *sim = p;
	Uint n = ckt->n;
	Uint m = ckt->m;

	SC_MatrixResize(sim->A, n+m, n+m);
	SC_MatrixResize(sim->G, n, n);
	SC_MatrixResize(sim->B, n, m);
	SC_MatrixResize(sim->C, m, n);
	SC_MatrixResize(sim->D, m, m);
	SC_MatrixResize(sim->LU, n+m, n+m);
	SC_IvectorResize(sim->piv, n+m);
	SC_MatrixResize(sim->z, n+m, 1);
	SC_MatrixResize(sim->i, n, 1);
	SC_MatrixResize(sim->e, m, 1);
	SC_MatrixResize(sim->x, n+m, 1);
	SC_MatrixResize(sim->v, n, 1);
	SC_MatrixResize(sim->j, m, 1);

	SC_MatrixSetZero(sim->A);
	SC_MatrixSetZero(sim->G);
	SC_MatrixSetZero(sim->B);
	SC_MatrixSetZero(sim->C);
	SC_MatrixSetZero(sim->D);
	SC_MatrixSetZero(sim->LU);
	SC_IvectorSetZero(sim->piv);
	SC_VectorSetZero(sim->z);
	SC_VectorSetZero(sim->i);
	SC_VectorSetZero(sim->e);
	SC_VectorSetZero(sim->x);
	SC_VectorSetZero(sim->v);
	SC_VectorSetZero(sim->j);
}

static void
Start(void *p)
{
	ES_SimDC *sim = p;
	ES_Circuit *ckt = SIM(sim)->ckt;
		
	Resize(sim, ckt);
	
	SIM(sim)->running = 1;
	AG_LockTimeouts(ckt);
	if (AG_TimeoutIsScheduled(ckt, &sim->update_to)) {
		AG_DelTimeout(ckt, &sim->update_to);
	}
	AG_AddTimeout(ckt, &sim->update_to, 1000/sim->speed);
	AG_UnlockTimeouts(ckt);
	sim->Telapsed = 0;
	
	ES_SimLog(sim, _("Simulation started (m=%u, n=%u)"), ckt->m, ckt->n);
}

static void
Stop(void *p)
{
	ES_SimDC *sim = p;
	ES_Circuit *ckt = SIM(sim)->ckt;

	AG_LockTimeouts(ckt);
	AG_DelTimeout(ckt, &sim->update_to);
	AG_UnlockTimeouts(ckt);
	SIM(sim)->running = 0;
	
	ES_SimLog(sim, _("Simulation stopped at %uns."), sim->Telapsed);
}

static void
Destroy(void *p)
{
	ES_SimDC *sim = p;
	
	Stop(sim);

	SC_MatrixFree(sim->A);
	SC_MatrixFree(sim->G);
	SC_MatrixFree(sim->B);
	SC_MatrixFree(sim->C);
	SC_MatrixFree(sim->D);
	SC_MatrixFree(sim->LU);
	SC_IvectorFree(sim->piv);
	SC_VectorFree(sim->z);
	SC_VectorFree(sim->i);
	SC_VectorFree(sim->e);
	SC_VectorFree(sim->x);
	SC_VectorFree(sim->v);
	SC_VectorFree(sim->j);
}

static void
RunSimulation(AG_Event *event)
{
	AG_Button *bu = AG_SELF();
	ES_Sim *sim = AG_PTR(1);
	int state = AG_INT(2);

	sim->ckt->simlock = 0;

	if (state) {
		Start(sim);
	} else {
		Stop(sim);
	}
}

static AG_Window *
Edit(void *p, ES_Circuit *ckt)
{
	ES_SimDC *sim = p;
	AG_Window *win;
	AG_Spinbutton *sbu;
	AG_FSpinbutton *fsu;
	AG_Tlist *tl;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;
	SC_Matview *mv;

	win = AG_WindowNew(AG_WINDOW_NOCLOSE);

	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);
	ntab = AG_NotebookAddTab(nb, _("Continuous mode"), AG_BOX_VERT);
	{
		AG_ButtonAct(ntab, AG_BUTTON_HFILL|AG_BUTTON_STICKY,
		    _("Run simulation"),
		    RunSimulation, "%p", sim);
	
		AG_SeparatorNew(ntab, AG_SEPARATOR_HORIZ);
		
		sbu = AG_SpinbuttonNew(ntab, 0,
		    _("Maximum iterations per step: "));
		AG_WidgetBind(sbu, "value", AG_WIDGET_UINT,
		    &sim->max_iters);

		sbu = AG_SpinbuttonNew(ntab, 0, _("Speed (updates/sec): "));
		AG_WidgetBind(sbu, "value", AG_WIDGET_UINT32, &sim->speed);
		AG_SpinbuttonSetRange(sbu, 1, 1000);
		
		AG_LabelNewPolled(ntab, 0,
		    _("Total simulated time: %[u32]ns"),
		    &sim->Telapsed);
		AG_LabelNewPolled(ntab, 0,
		    _("Realtime/step average: %[u32]ms"),
		    &sim->TavgReal);
		AG_LabelNewPolled(ntab, 0,
		    _("Iterations/step watermark: %u-%u"),
		    &sim->iters_lowat, &sim->iters_hiwat);
	}
	
	ntab = AG_NotebookAddTab(nb, "[A]", AG_BOX_VERT);
	mv = SC_MatviewNew(ntab, sim->A, 0);
	SC_MatviewSizeHint(mv, "-0.000", 4, 4);
	SC_MatviewSetNumericalFmt(mv, "%.02f");
	
	ntab = AG_NotebookAddTab(nb, "[LU]", AG_BOX_VERT);
	mv = SC_MatviewNew(ntab, sim->LU, 0);
	SC_MatviewSizeHint(mv, "-0.000", 10, 5);
	SC_MatviewSetNumericalFmt(mv, "%.02f");
#if 0
	ntab = AG_NotebookAddTab(nb, "[G]", AG_BOX_VERT);
	mv = SC_MatviewNew(ntab, sim->G, 0);
	SC_MatviewSizeHint(mv, "-0.000", 10, 5);
	SC_MatviewSetNumericalFmt(mv, "%.02f");
	
	ntab = AG_NotebookAddTab(nb, "[B]", AG_BOX_VERT);
	mv = SC_MatviewNew(ntab, sim->B, 0);
	SC_MatviewSizeHint(mv, "-0.000", 10, 5);
	SC_MatviewSetNumericalFmt(mv, "%.03f");
	
	ntab = AG_NotebookAddTab(nb, "[C]", AG_BOX_VERT);
	mv = SC_MatviewNew(ntab, sim->C, 0);
	SC_MatviewSizeHint(mv, "-0.000", 10, 5);
	SC_MatviewSetNumericalFmt(mv, "%.03f");
	
	ntab = AG_NotebookAddTab(nb, "[D]", AG_BOX_VERT);
	mv = SC_MatviewNew(ntab, sim->D, 0);
	SC_MatviewSizeHint(mv, "-0.000", 10, 5);
	SC_MatviewSetNumericalFmt(mv, "%.03f");
#endif
	ntab = AG_NotebookAddTab(nb, "[z]", AG_BOX_VERT);
	mv = SC_MatviewNew(ntab, sim->z, 0);
	SC_MatviewSizeHint(mv, "-0000.0000", 10, 5);
	SC_MatviewSetNumericalFmt(mv, "%.04f");
	
	ntab = AG_NotebookAddTab(nb, "[x]", AG_BOX_VERT);
	mv = SC_MatviewNew(ntab, sim->x, 0);
	SC_MatviewSizeHint(mv, "-0000.0000", 10, 5);
	SC_MatviewSetNumericalFmt(mv, "%.04f");
	
	return (win);
}

static SC_Real
NodeVoltage(void *p, int j)
{
	ES_SimDC *sim = p;

	return (vExists(sim->x,j) ? vEnt(sim->x,j) : 0.0);
}

static SC_Real
BranchCurrent(void *p, int k)
{
	ES_SimDC *sim = p;
	ES_Circuit *ckt = SIM(sim)->ckt;
	int i = ckt->n + k;

	return (vExists(sim->x,i) ? vEnt(sim->x,i) : 0.0);
}

const ES_SimOps esSimDcOps = {
	N_("DC Analysis"),
	&esIconSimDC,
	sizeof(ES_SimDC),
	Init,
	Destroy,
	Start,
	Stop,
	Resize,
	NodeVoltage,
	BranchCurrent,
	Edit
};
