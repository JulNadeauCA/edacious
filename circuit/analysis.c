/*	$Csoft: analysis.c,v 1.4 2005/05/25 07:03:56 vedge Exp $	*/

/*
 * Copyright (c) 2004, 2005 Winds Triton Engineering, Inc.
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

extern const struct analysis_ops kvl_ops;

const struct analysis_ops *analysis_ops[] = {
	&kvl_ops,
	NULL
};

void
analysis_init(void *p, const struct analysis_ops *ops)
{
	struct analysis *ana = p;

	ana->ops = ops;
	ana->running = 0;
}

void
analysis_destroy(void *p)
{
}

void
analysis_configure(int argc, union evarg *argv)
{
	struct circuit *ckt = argv[1].p;
	struct window *pwin = argv[2].p;
	struct analysis *ana = ckt->analysis;
	struct window *win;

	if (ana == NULL) {
		error_set(_("No analysis mode selected."));
		return;
	}
	if (ana->ops->settings != NULL &&
	    (win = ana->ops->settings(ana)) != NULL) {
		window_attach(pwin, win);
		window_show(win);
	}
}

