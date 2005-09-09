/*	$Csoft: rmsvoltage.c,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/

/*
 * Copyright (c) 2003, 2004, 2005 CubeSoft Communications, Inc.
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

static void rmsvoltage_init(void);

struct eda_tool rmsvoltage_tool = {
	N_("RMS Voltage"),
	4,
	rmsvoltage_init,
	NULL
};

static double rms = 1.0;
static double peak = 1.0;
static double avg = 1.0;

static void
rmsvoltage_update_peak(int argc, union evarg *argv)
{
	rms = 0.707 * peak;
	avg = 0.636 * peak;
}

static void
rmsvoltage_update_avg(int argc, union evarg *argv)
{
	rms = 1.11 * avg;
	peak = 1.57 * avg;
}

static void
rmsvoltage_init(void)
{
	struct window *win;
	struct fspinbutton *fsu;

	win = rmsvoltage_tool.win = window_new(WINDOW_NO_VRESIZE, "rmsvoltage");
	window_set_caption(win, rmsvoltage_tool.name);

	fsu = fspinbutton_new(win, "V", _("RMS: "));
	widget_bind(fsu, "value", WIDGET_DOUBLE, &rms);
	fspinbutton_set_writeable(fsu, 0);

	fsu = fspinbutton_new(win, "V", _("Peak: "));
	widget_bind(fsu, "value", WIDGET_DOUBLE, &peak);
	event_new(fsu, "fspinbutton-changed", rmsvoltage_update_peak, NULL);

	fsu = fspinbutton_new(win, "V", _("Average: "));
	widget_bind(fsu, "value", WIDGET_DOUBLE, &avg);
	event_new(fsu, "fspinbutton-changed", rmsvoltage_update_avg, NULL);
}

