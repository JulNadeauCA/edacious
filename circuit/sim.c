/*	$Csoft: sim.c,v 1.1 2005/09/08 09:46:01 vedge Exp $	*/

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

#include <engine/engine.h>

#include <engine/map/map.h>

#ifdef EDITION
#include <engine/widget/window.h>
#include <engine/widget/tlist.h>
#endif

#include <circuit/circuit.h>

extern const struct sim_ops kvl_ops;

const struct sim_ops *sim_ops[] = {
	&kvl_ops,
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
		view_detach(sim->win);
	}
	if (sim->ops->destroy != NULL) {
		sim->ops->destroy(sim);
	}
	Free(sim, M_EDA);
}

void
sim_edit(int argc, union evarg *argv)
{
	struct circuit *ckt = argv[1].p;
	struct window *pwin = argv[2].p;
	struct sim *sim = ckt->sim;
	struct window *win;

	if (sim == NULL) {
		error_set(_("No sim mode selected."));
		return;
	}
	if (sim->ops->edit != NULL &&
	    (win = sim->ops->edit(sim, ckt)) != NULL) {
		sim->win = win;
		window_attach(pwin, win);
		window_show(win);
	}
}

