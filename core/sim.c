/*
 * Copyright (c) 2004-2008 Hypertriton, Inc <http://hypertriton.com/>
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
 * Generic interface to different simulation modes.
 */

#include "core.h"
#include <stdarg.h>

extern const ES_SimOps esSimDcOps;

const ES_SimOps *esSimOps[] = {
	&esSimDcOps,
	NULL
};

void
ES_SimInit(void *p, const ES_SimOps *ops)
{
	ES_Sim *sim = p;

	sim->ops = ops;
	sim->running = 0;
	sim->win = NULL;
}

void
ES_SimDestroy(void *p)
{
	ES_Sim *sim = p;

	if (sim->win != NULL) {
		AG_ViewDetach(sim->win);
	}
	if (sim->ops->destroy != NULL) {
		sim->ops->destroy(sim);
	}
	Free(sim);
}

void
ES_SimEdit(AG_Event *event)
{
	ES_Circuit *ckt = AG_PTR(1);
	AG_Window *pwin = AG_PTR(2);
	ES_Sim *sim = ckt->sim;
	AG_Window *win;

	if (sim == NULL) {
		return;
	}
	if (sim->ops->edit != NULL &&
	    (win = sim->ops->edit(sim, ckt)) != NULL) {
		sim->win = win;
		AG_WindowAttach(pwin, win);
		AG_WindowShow(win);
	}
}

void
ES_SimLog(void *p, const char *fmt, ...)
{
	ES_Sim *sim = p;
	AG_ConsoleLine *ln;
	va_list args;

	if (sim->ckt == NULL || sim->ckt->console == NULL)
		return;

	ln = AG_ConsoleAppendLine(sim->ckt->console, NULL);
	va_start(args, fmt);
	AG_Vasprintf(&ln->text, fmt, args);
	va_end(args);
	ln->len = strlen(ln->text);
}

