/*	$Csoft: time.c,v 1.4 2004/09/18 06:38:43 vedge Exp $	*/

/*
 * Copyright (c) 2003, 2004 Winds Triton Engineering, Inc.
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
#include <engine/widget/window.h>
#include <engine/widget/box.h>
#include <engine/widget/fspinbutton.h>
#include <engine/widget/label.h>

#include <math.h>

#include "../tool.h"

static void time_init(void);

struct eda_tool time_tool = {
	N_("Time Equivalence"),
	10,
	time_init,
	NULL
};

static double curtime = 1.0;

static void
time_init(void)
{
	struct window *win;
	struct fspinbutton *fsu;

	win = time_tool.win = window_new(WINDOW_NO_VRESIZE, "time");
	window_set_caption(win, _("Time Equivalence"));

	fsu = fspinbutton_new(win, "sec", _("Time: "));
	widget_bind(fsu, "value", WIDGET_DOUBLE, &curtime);

	fsu = fspinbutton_new(win, "min", _("Time: "));
	widget_bind(fsu, "value", WIDGET_DOUBLE, &curtime);
}

