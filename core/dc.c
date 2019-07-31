/*
 * Copyright (c) 2008 Antoine Levitt (smeuuh@gmail.com)
 * Copyright (c) 2008 Steven Herbst (herbst@mit.edu)
 * Copyright (c) 2005-2009 Julien Nadeau (vedge@hypertriton.com)
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

#include "core.h"
#include <unistd.h>
#include <agar/math/m_matview.h>

/*
 * N-R iterations stop when :
 * V_n - V_{n-1} < MAX_V_DIFF + MAX_REL_DIFF * V_{n-1}
 * I_n - I_{n-1} < MAX_I_DIFF + MAX_REL_DIFF * I_{n-1}
 * */
#define MAX_REL_DIFF	1e-3	/* 0.1% */
#define MAX_V_DIFF	1e-6	/* 1 uV */
#define MAX_I_DIFF	1e-9	/* 1 pA */

/*
 * A relative LTE over MAX_REL_LTE will cause the step to
 * be rejected, and a LTE under MIN_REL_LTE will cause the
 * timestep to be increased
 * */
#define MAX_REL_LTE     0.01    /* 1% */
#define MIN_REL_LTE     0.0001  /* 0.01% */

/*
 * Maximum and minimum step size to be taken, in seconds */
#define MAX_TIMESTEP    1
#define MIN_TIMESTEP    1e-6

/* #define DC_DEBUG */

/* Solve the system of equations. */
static int
SolveMNA(ES_SimDC *sim, ES_Circuit *ckt)
{
	*sim->groundNode = 1.0;
	if (M_FactorizeLU(sim->A) == -1) {
		return (-1);
	}
	M_VecCopy(sim->x, sim->z);
	M_BacksubstLU(sim->A, sim->x); 
	return (0);
}

static void
StopSimulation(ES_SimDC *sim)
{
	SIM(sim)->running = 0;
	ES_SimLog(sim, _("Simulation stopped at %fs."), sim->Telapsed);
	AG_PostEvent(NULL, SIM(sim)->ckt, "circuit-sim-end", "%p", sim);
}

/*
 * Performs the NR loop. Returns the number of iterations performed,
 * the negative of iterations done if convergence was not achieved
 * and 0 if matrix is singular.
 */
static int
NR_Iterations(ES_Circuit *ckt, ES_SimDC *sim)
{
	ES_Component *com;
	Uint i = 0, j;

	{
	iter:
		if (++i > sim->itersMax)
			return -i; 

		sim->isDamped = 0;
		M_SetZero(sim->A);
		M_VecSetZero(sim->z);
		
		CIRCUIT_FOREACH_COMPONENT(com, ckt) {
			if (com->dcStepIter != NULL)
				com->dcStepIter(com, sim);
		}

		M_VecCopy(sim->xPrevIter, sim->x);
		if (SolveMNA(sim, ckt) == -1)
			return 0;

		/*
		 * Decide whether or not to exit the loop, based on
		 * the difference from previous iteration and the fact
		 * that the simulation may be damped
		 */
		if (sim->isDamped)
			goto iter;

		/* check for undefined voltages or current, which are a sign that the timestep is too large */
		for (j=0; j < (ckt->m+ckt->n); j++)
		{
			M_Real cur = M_VecGet(sim->x, j);
			if (M_IsNaN(cur) || M_IsInf(cur))
				return 0;
		}

		/* check difference on voltages and currents */
		/* voltages */

		for (j = 0; j < ckt->n; j++)
		{
			M_Real prev = M_VecGet(sim->xPrevIter, j);
			M_Real cur = M_VecGet(sim->x, j);

			if (Fabs(cur-prev) > MAX_V_DIFF+Fabs(MAX_REL_DIFF*prev))
				goto iter;
		}

		/* currents */
		for (j = ckt->n; j < ckt->m + ckt->n; j++)
		{
			M_Real prev = M_VecGet(sim->xPrevIter, j);
			M_Real cur = M_VecGet(sim->x, j);

			if (Fabs(cur-prev) > MAX_I_DIFF+Fabs(MAX_REL_DIFF*prev))
				goto iter;
		}
	}

	/* Update the statistics. */
	M_SetReal(ckt, "nrIters", i);
	if (i > sim->itersHigh) { sim->itersHigh = i; }
	if (i < sim->itersLow) { sim->itersLow = i; }
	
	return (i);
}

/* Change the current timestep. */
static void
SetTimestep(ES_SimDC *sim, M_Real deltaT)
{
	if (deltaT > MAX_TIMESTEP) { deltaT = MAX_TIMESTEP; }
	if (deltaT < MIN_TIMESTEP) { deltaT = MIN_TIMESTEP; }
	
	sim->deltaT = deltaT;

	/* Update the timestep statistics. */
	if (deltaT > sim->stepHigh) { sim->stepHigh = deltaT; }
	if (deltaT < sim->stepLow) { sim->stepLow = deltaT; }
}

/* Cycle the xPrevSteps and deltaTPrevSteps array : shift each solution one step back
 * in time, and forget the oldest one */
static void
CyclePreviousSolutions(ES_SimDC *sim)
{
	int i;
	M_Vector *last = sim->xPrevSteps[sim->stepsToKeep - 1];

	if (sim->stepsToKeep == 1) {
		M_VecSetZero(sim->xPrevSteps[0]);
		sim->deltaTPrevSteps[0] = 0;
		return;
	}

	for (i = sim->stepsToKeep-2; i >= 0; i--) {
		sim->xPrevSteps[i+1] = sim->xPrevSteps[i];
		sim->deltaTPrevSteps[i+1] = sim->deltaTPrevSteps[i];
	}
	sim->xPrevSteps[0] = last;
	sim->deltaTPrevSteps[0] = 0;
	M_VecSetZero(last);
}

/* Simulation timestep. */
static Uint32
StepMNA(AG_Timer *tm, AG_Event *event)
{
	ES_Circuit *ckt = AG_SELF();
	ES_SimDC *sim = AG_PTR(1);
	ES_Component *com;
	Uint retries;
	int i;
	M_Real error;

	sim->Telapsed += sim->deltaT;
	sim->currStep++;

	/* Notify the simulation objects of the beginning timestep. */
	for (i = 0; i < ckt->nExtObjs; i++)
		AG_PostEvent(ckt, ckt->extObjs[i], "circuit-step-begin", NULL);

stepbegin:
	sim->inputStep = 0;
	M_SetZero(sim->A);
	M_VecSetZero(sim->z);
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->dcStepBegin != NULL)
			com->dcStepBegin(com, sim);
	}
	
	/* DC biasing */
	if (SolveMNA(sim, ckt) == -1)
		goto halt;

	retries = 0;
	/* NR control loop : shrink timestep until a stable solution is found. */
	while (NR_Iterations(ckt,sim) <= 0) {
		/* NR_Iterations failed to converge : we reduce step size */
		if (++retries > sim->retriesMax) {
			AG_SetError(_("Could not find stable solution."));
			goto halt;
		}
#ifdef DC_DEBUG
		Debug(ckt,"NR failed to converge; timestep %g -> %g, "
		          "retry #%d\n", sim->deltaT, sim->deltaT/10,
			  retries);
#endif
		/* Undo last time step and and decimate deltaT. */
		sim->Telapsed -= sim->deltaT;
		SetTimestep(sim, sim->deltaT/10.0);
		sim->Telapsed += sim->deltaT;

		sim->inputStep = 1;
		M_SetZero(sim->A);
		M_VecSetZero(sim->z);
		CIRCUIT_FOREACH_COMPONENT(com, ckt) {
			if (com->dcStepBegin != NULL)
				com->dcStepBegin(com, sim);
		}
		if (SolveMNA(sim, ckt) == -1)
			goto halt;
	}

	/* Invoke the Component DC specific post-timestep callbacks. */
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->dcStepEnd != NULL)
			com->dcStepEnd(com, sim);
	}

	/* Get error from components */
	error = -1.0;
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->dcUpdateError != NULL)
			com->dcUpdateError(com, sim, &error);
	}
	/* No energy storage components : no error */
	if (error < 0.0) {
		error = 0.0;
	}
	M_SetReal(ckt, "%err", error*100);
	
	/* Do we accept this step ? */
	if (error > MAX_REL_LTE) {
#ifdef DC_DEBUG
		Debug(ckt, "LTE of %g, rejecting step; timestep %g -> %g\n",
		    error, sim->deltaT, sim->deltaT/2.0);
#endif
		sim->Telapsed -= sim->deltaT;
		SetTimestep(sim, sim->deltaT/2.0);
		sim->Telapsed += sim->deltaT;
		goto stepbegin;
	}
	
	/* Notify the simulation objects of the completed timestep. */
	for (i = 0; i < ckt->nExtObjs; i++)
		AG_PostEvent(ckt, ckt->extObjs[i], "circuit-step-end", NULL);

	/* Keep solution */
	CyclePreviousSolutions(sim);
	M_VecCopy(sim->xPrevSteps[0], sim->x);
	sim->deltaTPrevSteps[0] = sim->deltaT;

	/* Adjust timestep according to LTE */
	if (error < MIN_REL_LTE) {
#ifdef DC_DEBUG
		Debug(ckt, "LTE of %g, accepting step; timestep %g -> %g\n",
		    error, sim->deltaT, sim->deltaT*2.0);
#endif
		SetTimestep(sim, sim->deltaT*2.0);
	}
	
	/* Schedule next step */
	if (SIM(sim)->running) {
		return (sim->ticksDelay);
	} else {
		AG_DelTimer(ckt, &sim->toUpdate);
	}
	return (0);
halt:
	AG_TextMsg(AG_MSG_ERROR, _("%s; simulation stopped"), AG_GetError());
	StopSimulation(sim);
	return (0);
}

static void
ClearStats(ES_SimDC *sim)
{
	sim->itersLow = 1000;
	sim->itersHigh = 0;
	sim->stepLow = HUGE_VAL;
	sim->stepHigh = 0;
	sim->Telapsed = 0.0;
}

static void
Init(void *p)
{
	ES_SimDC *sim = p;

	ES_SimInit(sim, &esSimDcOps);
	//mMatOps = &mMatOps_FPU;

	sim->method = BE;
	sim->itersMax = 1000;
	sim->retriesMax = 25;
	sim->ticksDelay = 16;
	sim->currStep = 0;
	sim->T0 = 290.0;
	sim->A = M_New(0,0);
	sim->z = M_VecNew(0);
	sim->x = M_VecNew(0);
	sim->xPrevSteps = NULL;
	sim->deltaTPrevSteps = NULL;
	sim->stepsToKeep = 0;
	sim->xPrevIter = M_VecNew(0);
	sim->groundNode = NULL;

	AG_InitTimer(&sim->toUpdate, "stepMNA", 0);
	ClearStats(sim);
}

static void
InitMatrices(void *p, ES_Circuit *ckt)
{
	ES_SimDC *sim = p;
	Uint n = ckt->n;
	Uint m = ckt->m;
	int i;

	M_Resize(sim->A, n+m, n+m);
	M_SetZero(sim->A);
		
	M_VecResize(sim->z, n+m);
	M_VecResize(sim->x, n+m);
	M_VecResize(sim->xPrevIter, n+m);
	M_VecSetZero(sim->z);
	M_VecSetZero(sim->x);
	M_VecSetZero(sim->xPrevIter);

	sim->groundNode = M_GetElement(sim->A, 0, 0);

	/* Get number of steps to keep according to integration method. XXX */
	sim->stepsToKeep = 4;

	/* Initialise arrays */
	sim->xPrevSteps = Malloc(sim->stepsToKeep * sizeof(M_Vector *));
	for (i = 0; i < sim->stepsToKeep; i++) {
		sim->xPrevSteps[i] = M_VecNew(n+m);
		M_VecSetZero(sim->xPrevSteps[i]);
	}
	sim->deltaTPrevSteps = Malloc(sim->stepsToKeep * sizeof(M_Real));
	for (i = 0; i < sim->stepsToKeep; i++) {
		sim->deltaTPrevSteps[i] = 0.0;
	}
}

static void
Start(void *p)
{
	ES_SimDC *sim = p;
	ES_Circuit *ckt = SIM(sim)->ckt;
	ES_Component *com;

	/* Initialize vectors/matrices with proper size */
	InitMatrices(sim, ckt);

	/* Set the initial timing parameters and clear the statistics. */
	ClearStats(sim);
	sim->deltaT = ((M_Real) sim->ticksDelay)/1000.0;

	/* Invoke the DC-specific simulation start callback. */
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->dcSimBegin != NULL) {
			if (com->dcSimBegin(com, sim) == -1) {
				AG_SetError("%s: %s", OBJECT(com)->name,
				    AG_GetError());
				goto halt;
			}
		}
	}

	M_MNAPreorder(sim->A);

	/* Find the initial bias point. */
	if (SolveMNA(sim, ckt) == -1) {
		goto halt;
	}

	if (NR_Iterations(ckt,sim) <= 0) {
		AG_SetError("Failed to find initial bias point.");
		goto halt;
	}

	/* Keep solution */
	CyclePreviousSolutions(sim);
	M_VecCopy(sim->xPrevSteps[0], sim->x);
	sim->deltaTPrevSteps[0] = sim->deltaT;
	
	/* Schedule the call to StepMNA*/
	AG_LockTimers(ckt);
	if (AG_TimerIsRunning(ckt, &sim->toUpdate)) {
		AG_DelTimer(ckt, &sim->toUpdate);
	}
	AG_AddTimer(ckt, &sim->toUpdate, sim->ticksDelay,
	    StepMNA, "%p", sim);
	AG_UnlockTimers(ckt);

	SIM(sim)->running = 1;
	/* Invoke the general-purpose "simulation begin" callback. */
	AG_PostEvent(NULL, ckt, "circuit-sim-begin", "%p", sim);

	ES_SimLog(sim, _("Simulation started"));
	return;
halt:
	AG_TextMsg(AG_MSG_ERROR, _("%s; simulation stopped"), AG_GetError());
	StopSimulation(sim);
}

static void
Stop(void *p)
{
	ES_SimDC *sim = p;
	ES_Circuit *ckt = SIM(sim)->ckt;

	AG_LockTimers(ckt);
	AG_DelTimer(ckt, &sim->toUpdate);
	AG_UnlockTimers(ckt);

	StopSimulation(sim);
}

static void
Destroy(void *p)
{
	ES_SimDC *sim = p;
	int i;
	
	Stop(sim);

	M_Free(sim->A);
	M_VecFree(sim->z);
	M_VecFree(sim->x);
	M_VecFree(sim->xPrevIter);

	if (sim->xPrevSteps) {
		for (i = 0; i < sim->stepsToKeep; i++) {
			M_VecFree(sim->xPrevSteps[i]);
		}
		free(sim->xPrevSteps);
	}
	if (sim->deltaTPrevSteps)
		free(sim->deltaTPrevSteps);
}

static void
RunSimulation(AG_Event *event)
{
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
	AG_Notebook *nb;
	AG_NotebookTab *nt;
	M_Matview *mv;

	win = AG_WindowNew(AG_WINDOW_NOCLOSE);

	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);
	nt = AG_NotebookAdd(nb, _("Continuous mode"), AG_BOX_VERT);
	{
		AG_Button *btn;
		AG_Radio *rad;
		int i;

		btn = AG_ButtonNewFn(nt, AG_BUTTON_HFILL|AG_BUTTON_STICKY,
		    _("Run simulation"),
		    RunSimulation, "%p", sim);
		AG_BindInt(btn, "state", &SIM(sim)->running);

		AG_SeparatorNew(nt, AG_SEPARATOR_HORIZ);
	
		AG_NumericalNewUint(nt, 0, NULL, _("Refresh rate (delay): "), &sim->ticksDelay);
		AG_NumericalNewUint(nt, 0, NULL, _("Max. iterations/step: "), &sim->itersMax);

		rad = AG_RadioNewUint(nt, 0, NULL, &sim->method);
		for (i = 0; i < esIntegrationMethodCount; i++)
			AG_RadioAddItemS(rad, _(esIntegrationMethods[i].desc));

		AG_SeparatorNewHoriz(nt);

		AG_LabelNewPolled(nt, 0, _("Simulated time: %[T]"),
		    &sim->Telapsed);
		AG_LabelNewPolled(nt, 0, _("Timesteps: %[T] - %[T]"),
		    &sim->stepLow, &sim->stepHigh);
		AG_LabelNewPolled(nt, 0, _("Iterations: %u-%u"),
		    &sim->itersLow, &sim->itersHigh);
	}
	
	nt = AG_NotebookAdd(nb, _("Equations"), AG_BOX_VERT);
	AG_LabelNew(nt, 0, "A:");
	mv = M_MatviewNew(nt, sim->A, 0);
	M_MatviewSizeHint(mv, "-0.000", 4, 4);
	M_MatviewSetNumericalFmt(mv, "%.02f");
	AG_LabelNewPolled(nt, 0, "z: %[V]", &sim->z);
	AG_LabelNewPolled(nt, 0, "x: %[V]", &sim->x);

#if 0
	nt = AG_NotebookAdd(nb, "[z]", AG_BOX_VERT);
	mv = M_MatviewNew(nt, sim->z, 0);
	M_MatviewSizeHint(mv, "-0000.0000", 10, 5);
	M_MatviewSetNumericalFmt(mv, "%.04f");
	
	nt = AG_NotebookAdd(nb, "[x]", AG_BOX_VERT);
	mv = M_MatviewNew(nt, sim->x, 0);
	M_MatviewSizeHint(mv, "-0000.0000", 10, 5);
	M_MatviewSetNumericalFmt(mv, "%.04f");
#endif

	return (win);
}

static M_Real
NodeVoltage(void *p, int j)
{
	ES_SimDC *sim = p;

	return (j>=0)&&(sim->x->m > j) ? M_VecGet(sim->x, j) : 0.0;
}

static M_Real
NodeVoltagePrevStep(void *p, int j, int n)
{
	ES_SimDC *sim = p;

	if (n == 0) {
		return NodeVoltage(p, j);
	}
	return (j >= 0) && (sim->xPrevSteps[n-1]->m > j) ?
	       M_VecGet(sim->xPrevSteps[n-1],j) : 0.0;
}

static M_Real
BranchCurrent(void *p, int k)
{
	ES_SimDC *sim = p;
	ES_Circuit *ckt = SIM(sim)->ckt;
	int i = ckt->n + k;

	return (i >= 0) && (sim->x->m > i) ? M_VecGet(sim->x,i) : 0.0;
}

static M_Real
BranchCurrentPrevStep(void *p, int k, int n)
{
	ES_SimDC *sim = p;
	ES_Circuit *ckt = SIM(sim)->ckt;
	int i = ckt->n + k;

	if (n == 0) {
		return BranchCurrent(p,k);
	}
	return (i >= 0) && (sim->xPrevSteps[n-1]->m > i) ?
	       M_VecGet(sim->xPrevSteps[n-1],i) : 0.0;
}

const ES_SimOps esSimDcOps = {
	N_("DC Analysis"),
	&esIconSimDC,
	sizeof(ES_SimDC),
	Init,
	Destroy,
	Start,
	Stop,
	InitMatrices,
	NodeVoltage,
	NodeVoltagePrevStep,
	BranchCurrent,
	BranchCurrentPrevStep,
	Edit
};
