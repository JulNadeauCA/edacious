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

#include <eda.h>
#include <freesg/m/m_matview.h>

/* N-R iterations stop when the difference between previous
 * and current vector is no more than MAX_DIFF */
#define MAX_DIFF 0.001

/* Solve Ax=z where A=[G,B;C,D], x=[v,j] and z=[i;e]. */
static int
SolveMNA(ES_SimDC *sim, ES_Circuit *ckt)
{
	M_Real d;
	
	M_Compose21(sim->z, sim->i, sim->e);
	M_Compose22(sim->A, sim->G, sim->B, sim->C, sim->D);

	/* Find LU factorization and solve by backsubstitution. */
	M_FactorizeLU(sim->A, sim->LU, sim->piv, &d);
	M_Copy(sim->x, sim->z);
	M_BacksubstLU(sim->LU, sim->piv, sim->x);
	return (0);
}

static void
StopSimulation(ES_SimDC *sim)
{
	SIM(sim)->running = 0;
	ES_SimLog(sim, _("Simulation stopped at %fs."), sim->Telapsed);
	AG_PostEvent(NULL, SIM(sim)->ckt, "circuit-sim-end", "%p", sim);
}

/* Simulation timestep. */
static Uint32
StepMNA(void *obj, Uint32 ival, void *arg)
{
	ES_Circuit *ckt = obj;
	ES_SimDC *sim = arg;
	ES_Component *com;
	M_Vector *xPrev;
	M_Real diff;
	Uint i = 0, j;
	Uint32 ticks;
	Uint32 ticksSinceLast;
	
	ticks = SDL_GetTicks();
	ticksSinceLast = ticks - sim->timeLastStep;
	sim->deltaT = ticksSinceLast / 1000.0;
	sim->Telapsed += sim->deltaT;
	sim->timeLastStep = ticks;

	xPrev = M_New(sim->x->m, 1);

	if (ckt->n == 0) {
		AG_SetError(_("Circuit has no nodes to evaluate"));
		goto halt;
	}

	/* Invoke the general pre-timestep callbacks (for ES_Scope, etc.) */
	AG_PostEvent(NULL, ckt, "circuit-step-begin", NULL);

	/* Formulate and solve for the initial conditions. */
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->dcStepBegin != NULL)
			com->dcStepBegin(com, sim);
	}
	if (SolveMNA(sim, ckt) == -1)
		goto halt;

	/* Iterate until a stable solution is found. */
	do {
		if (++i > sim->itersMax) {
#if 1
			AG_SetError(_("Could not find stable solution in "
			              "%u iterations"), i);
			goto halt;
#endif
			break;
		}

		M_Copy(xPrev, sim->x);
		CIRCUIT_FOREACH_COMPONENT(com, ckt) {
			if (com->dcStepIter != NULL)
				com->dcStepIter(com, sim);
		}
		if (SolveMNA(sim, ckt) == -1)
			goto halt;

		/* Compute difference between previous and current iteration,
		 * to decide whether or not to continue. */
		diff = 0;
		for (j = 0; j < sim->x->m; j++)
		{
			M_Real curAbsDiff = Fabs(sim->x->v[j][0] -
			                         xPrev->v[j][0]);
			if (curAbsDiff > diff)
				diff = curAbsDiff;
		}
#ifdef DEBUG
		M_SetReal(ckt, "dcDiff", diff);
#endif
	} while (diff > MAX_DIFF);
#ifdef DEBUG
	M_SetReal(ckt, "dcIters", i);
#endif

	/* Invoke the Component DC specific post-timestep callbacks. */
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->dcStepEnd != NULL)
			com->dcStepEnd(com, sim);
	}

	/* Update the statistics. */
	if (i > sim->itersHiwat) { sim->itersHiwat = i; }
	else if (i < sim->itersLowat) { sim->itersLowat = i; }

	/* Invoke the general post-timestep callbacks (for ES_Scope, etc.) */
	AG_PostEvent(NULL, ckt, "circuit-step-end", NULL);

	M_Free(xPrev);

	/* Schedule next step */
	if(SIM(sim)->running) {
		Uint32 nextStep = sim->timeLastStep + 1000/sim->maxSpeed;
		Uint32 newTicks = SDL_GetTicks();
		return nextStep > newTicks ? nextStep - newTicks : 0;
	}
	else {
		AG_LockTimeouts(ckt);
		AG_DelTimeout(ckt, &sim->toUpdate);
		AG_UnlockTimeouts(ckt);
	}
	
	return (0);
halt:
	AG_TextMsg(AG_MSG_ERROR, _("%s; simulation stopped"), AG_GetError());
	StopSimulation(sim);
	M_Free(xPrev);
	return (0);
}

static void
Init(void *p)
{
	ES_SimDC *sim = p;

	sim->method = BE;
	
	ES_SimInit(sim, &esSimDcOps);
	AG_SetTimeout(&sim->toUpdate, StepMNA, sim, AG_CANCEL_ONDETACH);
	sim->itersMax = 10000;
	sim->itersHiwat = 1;
	sim->itersLowat = 1;

	sim->Telapsed = 0.0;
	sim->maxSpeed = 60;
	sim->timeLastStep = 0.0;
	
	sim->T0 = 290.0;

	sim->A = M_New(0,0);
	sim->G = M_New(0,0);
	sim->B = M_New(0,0);
	sim->C = M_New(0,0);
	sim->D = M_New(0,0);
	sim->LU = M_New(0,0);

	sim->z = M_New(0,0);
	sim->i = M_New(0,0);
	sim->e = M_New(0,0);
	
	sim->x = M_New(0,0);
	sim->v = M_New(0,0);
	sim->j = M_New(0,0);

	sim->piv = M_IntVectorNew(0);
}

static void
Resize(void *p, ES_Circuit *ckt)
{
	ES_SimDC *sim = p;
	Uint n = ckt->n;
	Uint m = ckt->m;

	M_Resize(sim->A, n+m, n+m);
	M_Resize(sim->G, n, n);
	M_Resize(sim->B, n, m);
	M_Resize(sim->C, m, n);
	M_Resize(sim->D, m, m);
	M_Resize(sim->LU, n+m, n+m);
	
	M_IntVectorResize(sim->piv, n+m);
	M_Resize(sim->z, n+m, 1);
	M_Resize(sim->i, n, 1);
	M_Resize(sim->e, m, 1);
	M_Resize(sim->x, n+m, 1);
	M_Resize(sim->v, n, 1);
	M_Resize(sim->j, m, 1);

	M_SetZero(sim->A);
	M_SetZero(sim->G);
	M_SetZero(sim->B);
	M_SetZero(sim->C);
	M_SetZero(sim->D);
	M_SetZero(sim->LU);
	
	M_IntVectorSet(sim->piv, 0);
	
	M_SetZero(sim->z);
	M_SetZero(sim->i);
	M_SetZero(sim->e);
	
	M_SetZero(sim->x);
	M_SetZero(sim->v);
	M_SetZero(sim->j);
}

static void
Start(void *p)
{
	ES_SimDC *sim = p;
	ES_Circuit *ckt = SIM(sim)->ckt;
	ES_Component *com;

	/* Reallocate the matrices if necessary. */
	Resize(sim, ckt);

	SIM(sim)->running = 1;

	AG_LockTimeouts(ckt);
	if (AG_TimeoutIsScheduled(ckt, &sim->toUpdate)) {
		AG_DelTimeout(ckt, &sim->toUpdate);
	}
	AG_AddTimeout(ckt, &sim->toUpdate, 1000/sim->maxSpeed);
	AG_UnlockTimeouts(ckt);
	sim->Telapsed = 0.0;
	sim->timeLastStep = SDL_GetTicks();
	/* arbitrary value for initialisation */
	sim->deltaT = 1.0/sim->maxSpeed;
	
	/* Initialize the matrices. */
	M_SetZero(sim->G);
	M_SetZero(sim->B);
	M_SetZero(sim->C);
	M_SetZero(sim->D);
	M_SetZero(sim->i);
	M_SetZero(sim->e);

	/* Load datum node equation */
	sim->G->v[0][0] = 1.0;
	
	/*
	 * Invoke the DC-specific simulation start callback. Components can
	 * use this callback to verify constant parameters.
	 */
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->dcSimBegin != NULL) {
			if (com->dcSimBegin(com, sim) == -1) {
				AG_SetError("%s: %s", OBJECT(com)->name,
				    AG_GetError());
				goto halt;
			}
		}
	}

	if (SolveMNA(sim, ckt) == -1)
		goto halt;

	/* Invoke the general simulation start callback. */
	AG_PostEvent(NULL, ckt, "circuit-sim-begin", "%p", sim);
	
	ES_SimLog(sim, _("Simulation started (m=%u, n=%u)"), ckt->m, ckt->n);
	return;
halt:
	SIM(sim)->running = 0;
	ES_SimLog(sim, "%s", AG_GetError());
}

static void
Stop(void *p)
{
	ES_SimDC *sim = p;
	ES_Circuit *ckt = SIM(sim)->ckt;
	AG_LockTimeouts(ckt);
	AG_DelTimeout(ckt, &sim->toUpdate);
	AG_UnlockTimeouts(ckt);
	StopSimulation(sim);
}

static void
Destroy(void *p)
{
	ES_SimDC *sim = p;
	
	Stop(sim);

	M_Free(sim->A);
	M_Free(sim->G);
	M_Free(sim->B);
	M_Free(sim->C);
	M_Free(sim->D);
	M_Free(sim->LU);
	M_IntVectorFree(sim->piv);
	M_Free(sim->z);
	M_Free(sim->i);
	M_Free(sim->e);
	M_Free(sim->x);
	M_Free(sim->v);
	M_Free(sim->j);
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
	M_Matview *mv;

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
		    &sim->itersMax);

		sbu = AG_SpinbuttonNew(ntab, 0, _("Max speed (updates/sec): "));
		AG_WidgetBind(sbu, "value", AG_WIDGET_UINT32, &sim->maxSpeed);
		AG_SpinbuttonSetRange(sbu, 1, 1000);
		
		AG_LabelNewPolled(ntab, 0,
		    _("Total simulated time: %fs"),
		    &sim->Telapsed);
		AG_LabelNewPolled(ntab, 0,
		    _("Iterations/step watermark: %u-%u"),
		    &sim->itersLowat, &sim->itersHiwat);
	}
	
	ntab = AG_NotebookAddTab(nb, "[A]", AG_BOX_VERT);
	mv = M_MatviewNew(ntab, sim->A, 0);
	M_MatviewSizeHint(mv, "-0.000", 4, 4);
	M_MatviewSetNumericalFmt(mv, "%.02f");
	
	ntab = AG_NotebookAddTab(nb, "[LU]", AG_BOX_VERT);
	mv = M_MatviewNew(ntab, sim->LU, 0);
	M_MatviewSizeHint(mv, "-0.000", 10, 5);
	M_MatviewSetNumericalFmt(mv, "%.02f");
#if 0
	ntab = AG_NotebookAddTab(nb, "[G]", AG_BOX_VERT);
	mv = M_MatviewNew(ntab, sim->G, 0);
	M_MatviewSizeHint(mv, "-0.000", 10, 5);
	M_MatviewSetNumericalFmt(mv, "%.02f");
	
	ntab = AG_NotebookAddTab(nb, "[B]", AG_BOX_VERT);
	mv = M_MatviewNew(ntab, sim->B, 0);
	M_MatviewSizeHint(mv, "-0.000", 10, 5);
	M_MatviewSetNumericalFmt(mv, "%.03f");
	
	ntab = AG_NotebookAddTab(nb, "[C]", AG_BOX_VERT);
	mv = M_MatviewNew(ntab, sim->C, 0);
	M_MatviewSizeHint(mv, "-0.000", 10, 5);
	M_MatviewSetNumericalFmt(mv, "%.03f");
	
	ntab = AG_NotebookAddTab(nb, "[D]", AG_BOX_VERT);
	mv = M_MatviewNew(ntab, sim->D, 0);
	M_MatviewSizeHint(mv, "-0.000", 10, 5);
	M_MatviewSetNumericalFmt(mv, "%.03f");
#endif

	ntab = AG_NotebookAddTab(nb, "[z]", AG_BOX_VERT);
	mv = M_MatviewNew(ntab, sim->z, 0);
	M_MatviewSizeHint(mv, "-0000.0000", 10, 5);
	M_MatviewSetNumericalFmt(mv, "%.04f");
	
	ntab = AG_NotebookAddTab(nb, "[x]", AG_BOX_VERT);
	mv = M_MatviewNew(ntab, sim->x, 0);
	M_MatviewSizeHint(mv, "-0000.0000", 10, 5);
	M_MatviewSetNumericalFmt(mv, "%.04f");

	return (win);
}

static M_Real
NodeVoltage(void *p, int j)
{
	ES_SimDC *sim = p;

	return M_VEC_ENTRY_EXISTS(sim->x,j) ? sim->x->v[j][0] : 0.0;
}

static M_Real
BranchCurrent(void *p, int k)
{
	ES_SimDC *sim = p;
	ES_Circuit *ckt = SIM(sim)->ckt;
	int i = ckt->n + k;

	return M_VEC_ENTRY_EXISTS(sim->x,i) ? sim->x->v[i][0] : 0.0;
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
