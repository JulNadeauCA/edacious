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

#include "core.h"
#include <unistd.h>
#include <freesg/m/m_matview.h>

/*
 * N-R iterations stop when :
 * V_n - V_{n-1} < MAX_V_DIFF + MAX_REL_DIFF * V_{n-1}
 * I_n - I_{n-1} < MAX_I_DIFF + MAX_REL_DIFF * I_{n-1}
 * */
#define MAX_REL_DIFF	1e-3	/* 0.1% */
#define MAX_V_DIFF	1e-6	/* 1 uV */
#define MAX_I_DIFF	1e-12	/* 1 pA */

static void
MatrixDebug(ES_SimDC *sim, ES_Circuit *ckt, char *str)
{
	Debug(ckt, "%s\n", str);
	Debug(ckt, "A:\n");
	M_MatrixPrint(sim->A);
	Debug(ckt, "x:\n");
	M_VectorPrint(sim->x);
	Debug(ckt, "z:\n");
	M_VectorPrint(sim->z);
}

/* Solve Ax=z where A=[G,B;C,D], x=[v,j] and z=[i;e]. */
static int
SolveMNA(ES_SimDC *sim, ES_Circuit *ckt)
{
	*sim->groundNode = 1.0;
	/* Find LU factorization and solve by backsubstitution. */
	M_FactorizeLU(sim->A);
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
	M_Real diff;
	Uint i = 0, j, loop;

	while(1) {
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
		if(sim->isDamped)
			continue;
		

		/* check difference on voltages and currents */
		/* voltages */

		loop=0;
		for (j = 0; j < ckt->n; j++)
		{
			M_Real prev = M_VecGet(sim->xPrevIter, j);
			M_Real cur = M_VecGet(sim->x, j);
			if(Fabs(cur - prev) > MAX_V_DIFF + Fabs(MAX_REL_DIFF * prev))
			{
				loop=1;
				break;
			}
		}
		if (loop)
			continue;

		/* currents */
		loop=0;
		for (j = ckt->n; j < ckt->m + ckt->n; j++)
		{
			M_Real prev = M_VecGet(sim->xPrevIter, j);
			M_Real cur = M_VecGet(sim->x, j);
			if(Fabs(cur - prev) > MAX_I_DIFF + Fabs(MAX_REL_DIFF * prev))
			{
				loop=1;
				break;
			}
		}
		if (loop)
			continue;

		break;
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
	if(sim->stepsToKeep == 1)
	{
		M_VecSetZero(sim->xPrevSteps[0]);
		sim->deltaTPrevSteps[0] = 0;
		return;
	}
	for(i = sim->stepsToKeep - 2 ; i >= 0 ; i--)
	{
		sim->xPrevSteps[i+1] = sim->xPrevSteps[i];
		sim->deltaTPrevSteps[i+1] = sim->deltaTPrevSteps[i];
	}
	sim->xPrevSteps[0] = last;
	sim->deltaTPrevSteps[0] = 0;
	M_VecSetZero(last);
}

/* Simulation timestep. */
static Uint32
StepMNA(void *obj, Uint32 ival, void *arg)
{
	ES_Circuit *ckt = obj;
	ES_SimDC *sim = arg;
	ES_Component *com;
	Uint retries=0;
	Uint32 ticks;
	Uint32 ticksSinceLast;
	int i;
	M_Real error;

	/* Get time since last step and set that to be our deltaT */
	ticks = SDL_GetTicks();
	ticksSinceLast = ticks - sim->ticksLastStep;
	SetTimestep(sim, (M_Real)(ticksSinceLast/1000.0));
	sim->Telapsed += sim->deltaT;
	sim->ticksLastStep = ticks;

	/* Notify the simulation objects of the beginning timestep. */
	for (i = 0; i < ckt->nExtObjs; i++)
		AG_PostEvent(ckt, ckt->extObjs[i], "circuit-step-begin", NULL);

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

	/* NR control loop : shrink timestep until a stable solution is found. */
	while (NR_Iterations(ckt,sim) <= 0) {
		/* NR_Iterations failed to converge : we reduce step size */
	
		if (++retries > sim->retriesMax) {
			AG_SetError(_("Could not find stable solution."));
			goto halt;
		}

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

	/* Get error from components */
	error = HUGE_VAL;
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->dcUpdateError != NULL)
			com->dcUpdateError(com, sim, &error);
	}
	/* No energy storage components : no error */
	if(error == HUGE_VAL)
		error = 0;
	M_SetReal(ckt, "%err", error*100);

	/* Invoke the Component DC specific post-timestep callbacks. */
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->dcStepEnd != NULL)
			com->dcStepEnd(com, sim);
	}

	/* Notify the simulation objects of the completed timestep. */
	for (i = 0; i < ckt->nExtObjs; i++)
		AG_PostEvent(ckt, ckt->extObjs[i], "circuit-step-end", NULL);

	/* Keep solution */
	CyclePreviousSolutions(sim);
	M_VecCopy(sim->xPrevSteps[0], sim->x);
	sim->deltaTPrevSteps[0] = sim->deltaT;

	/* Schedule next step */
	if (SIM(sim)->running) {
		Uint32 nextStep = sim->ticksLastStep + sim->ticksDelay;
		Uint32 newTicks = SDL_GetTicks();
		return nextStep > newTicks ? nextStep - newTicks : 1;
	} else {
		AG_LockTimeouts(ckt);
		AG_DelTimeout(ckt, &sim->toUpdate);
		AG_UnlockTimeouts(ckt);
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
	sim->ticksLastStep = 0.0;
}

static void
Init(void *p)
{
	ES_SimDC *sim = p;

	ES_SimInit(sim, &esSimDcOps);

	sim->method = BE;
	sim->itersMax = 1000;
	sim->retriesMax = 25;
	sim->ticksDelay = 16;
	sim->T0 = 290.0;
	sim->A = M_New(0,0);
	sim->z = M_VecNew(0);
	sim->x = M_VecNew(0);
	sim->xPrevSteps = NULL;
	sim->stepsToKeep = 0;
	sim->xPrevIter = M_VecNew(0);
	sim->groundNode = NULL;
	
	AG_SetTimeout(&sim->toUpdate, StepMNA, sim, AG_CANCEL_ONDETACH);
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

	/* Get number of steps to keep according to integration method */
	sim->stepsToKeep = 4;
	/* Initialise arrays */
	sim->xPrevSteps = malloc(sim->stepsToKeep * sizeof(M_Vector *));
	for(i = 0; i < sim->stepsToKeep ; i++)
	{
		sim->xPrevSteps[i] = M_VecNew(n+m);
		M_VecSetZero(sim->xPrevSteps[i]);
	}
	sim->deltaTPrevSteps = malloc(sim->stepsToKeep * sizeof(M_Real));
	for(i = 0; i < sim->stepsToKeep ; i++)
	{
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
	sim->ticksLastStep = SDL_GetTicks();
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
	AG_LockTimeouts(ckt);
	if (AG_TimeoutIsScheduled(ckt, &sim->toUpdate)) {
		AG_DelTimeout(ckt, &sim->toUpdate);
	}
	AG_AddTimeout(ckt, &sim->toUpdate, sim->ticksDelay);
	AG_UnlockTimeouts(ckt);

	SIM(sim)->running = 1;
	/* Invoke the general-purpose "simulation begin" callback. */
	AG_PostEvent(NULL, ckt, "circuit-sim-begin", "%p", sim);

	ES_SimLog(sim, _("Simulation started"));
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
	M_VecFree(sim->z);
	M_VecFree(sim->x);
	M_VecFree(sim->xPrevIter);

	if(sim->xPrevSteps)
	{
		int i;
		for(i = 0; i < sim->stepsToKeep ; i++)
		{
			M_VecFree(sim->xPrevSteps[i]);
		}
		free(sim->xPrevSteps);
	}
	if(sim->deltaTPrevSteps)
		free(sim->deltaTPrevSteps);
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
	AG_Radio *r;
	AG_Notebook *nb;
	AG_NotebookTab *ntab;
	M_Matview *mv;

	win = AG_WindowNew(AG_WINDOW_NOCLOSE);

	nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);
	ntab = AG_NotebookAddTab(nb, _("Continuous mode"), AG_BOX_VERT);
	{
		const char *methodNames[] = {
			N_("Backward Euler"),
			N_("Forward Euler"),
			N_("Trapezoidal"),
			NULL
		};

		AG_ButtonAct(ntab, AG_BUTTON_HFILL|AG_BUTTON_STICKY,
		    _("Run simulation"),
		    RunSimulation, "%p", sim);
	
		AG_SeparatorNew(ntab, AG_SEPARATOR_HORIZ);
	
		AG_NumericalNewUint(ntab, 0, NULL, _("Refresh rate (delay): "),
		    &sim->ticksDelay);
		AG_NumericalNewUint(ntab, 0, NULL, _("Max. iterations/step: "),
		    &sim->itersMax);

		r = AG_RadioNew(ntab, 0, methodNames);
		AG_WidgetBind(r, "value", AG_WIDGET_UINT, &sim->method);

		AG_SeparatorNewHoriz(ntab);

		AG_LabelNewPolled(ntab, 0, _("Simulated time: %[T]"),
		    &sim->Telapsed);
		AG_LabelNewPolled(ntab, 0, _("Timesteps: %[T] - %[T]"),
		    &sim->stepLow, &sim->stepHigh);
		AG_LabelNewPolled(ntab, 0, _("Iterations: %u-%u"),
		    &sim->itersLow, &sim->itersHigh);
	}
	
	ntab = AG_NotebookAddTab(nb, _("Equations"), AG_BOX_VERT);
	AG_LabelNew(ntab, 0, "A:");
	mv = M_MatviewNew(ntab, sim->A, 0);
	M_MatviewSizeHint(mv, "-0.000", 4, 4);
	M_MatviewSetNumericalFmt(mv, "%.02f");
	AG_LabelNewPolled(ntab, 0, "z: %[V]", &sim->z);
	AG_LabelNewPolled(ntab, 0, "x: %[V]", &sim->x);

#if 0
	ntab = AG_NotebookAddTab(nb, "[z]", AG_BOX_VERT);
	mv = M_MatviewNew(ntab, sim->z, 0);
	M_MatviewSizeHint(mv, "-0000.0000", 10, 5);
	M_MatviewSetNumericalFmt(mv, "%.04f");
	
	ntab = AG_NotebookAddTab(nb, "[x]", AG_BOX_VERT);
	mv = M_MatviewNew(ntab, sim->x, 0);
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

	return (j>=0)&&(sim->xPrevSteps[n-1]->m > j) ? M_VecGet(sim->xPrevSteps[n-1], j) : 0.0;
}

static M_Real
BranchCurrent(void *p, int k)
{
	ES_SimDC *sim = p;
	ES_Circuit *ckt = SIM(sim)->ckt;
	int i = ckt->n + k;

	return (i>=0)&&(sim->x->m > i) ? M_VecGet(sim->x, i) : 0.0;
}

static M_Real
BranchCurrentPrevStep(void *p, int k, int n)
{
	ES_SimDC *sim = p;
	ES_Circuit *ckt = SIM(sim)->ckt;
	int i = ckt->n + k;

	return (i>=0)&&(sim->xPrevSteps[n-1]->m > i) ? M_VecGet(sim->xPrevSteps[n-1], i) : 0.0;
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
