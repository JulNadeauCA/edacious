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

/* Formulate the KCL/branch equations and compute A=LU. */
static int
FactorizeMNA(ES_SimDC *sim, ES_Circuit *ckt)
{
	ES_Component *com;
	Uint i, n, m;
	M_Real d;

	M_SetZero(sim->G);
	M_SetZero(sim->B);
	M_SetZero(sim->C);
	M_SetZero(sim->D);
	M_SetZero(sim->i);
	M_SetZero(sim->e);

	sim->G->v[0][0] = 1.0;

	/* Formulate the general equations. */
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
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
	M_Compose21(sim->z, sim->i, sim->e);

	/* Compose the coefficient matrix A from [G,B;C,D] and compute LU. */
	M_Compose22(sim->A, sim->G, sim->B, sim->C, sim->D);

	fprintf(stderr, "Solving:\n");
	M_MatrixPrint(sim->A);

	return ((M_FactorizeLU(sim->A, sim->LU, sim->piv, &d)) != NULL) ?
	        0 : -1;
}

static int
SolveMNA(ES_SimDC *sim, ES_Circuit *ckt)
{
	M_Copy(sim->x, sim->z);
	M_BacksubstLU(sim->LU, sim->piv, sim->x);
	return (0);
}

static Uint32
StepMNA(void *obj, Uint32 ival, void *arg)
{
	ES_Circuit *ckt = obj;
	ES_SimDC *sim = arg;
	ES_Component *com;
	static Uint32 t1 = 0;
	M_Matrix *Aprev;
	M_Real diff;
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
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->intStep != NULL)
			com->intStep(com, sim->Telapsed);
		if (com->dcStep != NULL)
			com->dcUpdate(com, sim->G, sim->B, sim->C, sim->D, sim->i, sim->e, sim->v, sim->j);
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
	Aprev = M_Dup(sim->A);
	CIRCUIT_FOREACH_COMPONENT(com, ckt) {
		if (com->intUpdate != NULL)
			com->intUpdate(com);
		if (com->dcUpdate != NULL)
			com->dcUpdate(com, sim->G, sim->B, sim->C, sim->D, sim->i, sim->e, sim->v, sim->j);

	}
	if (FactorizeMNA(sim, ckt) == -1 ||
	    SolveMNA(sim, ckt) == -1) {
		goto halt;
	}
	/* Loop until stability is reached. */
	M_Compare(sim->A, Aprev, &diff);
	if (diff > M_MACHEP) {
		if (++i > sim->max_iters) {
			AG_SetError(_("Could not find stable solution in "
			              "%u iterations"), i);
			goto halt;
		}
		goto solve;
	}
	if (i > sim->iters_hiwat) { sim->iters_hiwat = i; }
	else if (i < sim->iters_lowat) { sim->iters_lowat = i; }

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
	sim->max_iters = 10000;
	sim->iters_hiwat = 1;
	sim->iters_lowat = 1;
	AG_SetTimeout(&sim->update_to, StepMNA, sim, AG_CANCEL_ONDETACH);

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
