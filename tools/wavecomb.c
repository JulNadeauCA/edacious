/*	$Csoft: wavecomb.c,v 1.2 2004/09/12 05:58:08 vedge Exp $	*/

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
#include <engine/widget/mgraph.h>

#include <math.h>

#include "../tool.h"

static void wavecomb_init(void);

struct eda_tool wavecomb_tool = {
	N_("Waveform Combination"),
	6,
	wavecomb_init,
	NULL
};

static double
sin_wave(double x)
{
	return (sin(x));
}

static void
wavecomb_init(void)
{
	struct window *win;
	struct mgraph *gra;

	win = wavecomb_tool.win = window_new(0, "wavecomb");
	window_set_caption(win, _("Waveform Combination"));

	gra = mgraph_new(win, MGRAPH_LINES, _("Waveform 1"));
	mgraph_add_function(gra, sin_wave, 300, 250, 250, 250, _("Amplitude"));
}

