/*	$Csoft: sim.c,v 1.3 2005/09/10 05:48:20 vedge Exp $	*/

/*
 * Copyright (c) 2004, 2005 CubeSoft Communications Inc
 * <http://www.winds-triton.com>
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

#include "eda.h"

extern const struct sim_ops kvl_ops;
extern const struct sim_ops mna_ops;

const struct sim_ops *sim_ops[] = {
	&kvl_ops,
	&mna_ops,
	NULL
};

void
sim_init(void *p, const struct sim_ops *ops)
{
	struct sim *sim = p;

	sim->ops = ops;
	sim->running = 0;
	sim->win = NULL;
}

void
sim_destroy(void *p)
{
	struct sim *sim = p;

	if (sim->win != NULL) {
		AG_ViewDetach(sim->win);
	}
	if (sim->ops->destroy != NULL) {
		sim->ops->destroy(sim);
	}
	Free(sim, M_EDA);
}

void
sim_edit(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	AG_Window *pwin = AG_PTR(2);
	struct sim *sim = ckt->sim;
	AG_Window *win;

	if (sim == NULL) {
		AG_SetError(_("No sim mode selected."));
		return;
	}
	if (sim->ops->edit != NULL &&
	    (win = sim->ops->edit(sim, ckt)) != NULL) {
		sim->win = win;
		AG_WindowAttach(pwin, win);
		AG_WindowShow(win);
	}
}

