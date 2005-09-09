/*	$Csoft: impedance.c,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/

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

static void impedance_init(void);

struct eda_tool impedance_tool = {
	N_("Impedance"),
	11,
	impedance_init,
	NULL
};

static double capacitive_reactance = 1.0;
static double inductive_reactance = 1.0;
static double impedance = 1.0;
static double frequency = 60.0;
static double capacitance = 0;
static double inductance = 0;
static double resistance = 0;

static void
impedance_update(int argc, union evarg *argv)
{
	capacitive_reactance = 1 / (2 * M_PI * frequency * capacitance);
	inductive_reactance = 2 * M_PI * frequency * inductance;

	if (!isinf(capacitive_reactance) &&
	    isinf(inductive_reactance)) {
		impedance = capacitive_reactance;
	} else if (!isinf(inductive_reactance) &&
	           isinf(capacitive_reactance)) {
		impedance = inductive_reactance;
	} else if (!isinf(inductive_reactance) &&
	           !isinf(capacitive_reactance)) {
		double reactance;

		reactance = inductive_reactance - capacitive_reactance;
		impedance = sqrt(pow(resistance,2) + pow(reactance,2));
	} else {
		impedance = 0;
	}
}

static void
impedance_init(void)
{
	struct window *win;
	struct fspinbutton *fsu;

	win = impedance_tool.win = window_new(WINDOW_NO_VRESIZE, "impedance");
	window_set_caption(win, _("Impedance"));

	fsu = fspinbutton_new(win, "ohms", _("Capacitive reactance: "));
	widget_bind(fsu, "value", WIDGET_DOUBLE, &capacitive_reactance);
	fspinbutton_set_writeable(fsu, 0);
	
	fsu = fspinbutton_new(win, "ohms", _("Inductive reactance: "));
	widget_bind(fsu, "value", WIDGET_DOUBLE, &inductive_reactance);
	fspinbutton_set_writeable(fsu, 0);
	
	fsu = fspinbutton_new(win, "ohms", _("Impedance: "));
	widget_bind(fsu, "value", WIDGET_DOUBLE, &impedance);
	fspinbutton_set_writeable(fsu, 0);
	
	fsu = fspinbutton_new(win, "Hz", _("Frequency: "));
	widget_bind(fsu, "value", WIDGET_DOUBLE, &frequency);
	event_new(fsu, "fspinbutton-changed", impedance_update, NULL);

	fsu = fspinbutton_new(win, "uF", _("Capacitance: "));
	widget_bind(fsu, "value", WIDGET_DOUBLE, &capacitance);
	event_new(fsu, "fspinbutton-changed", impedance_update, NULL);
	
	fsu = fspinbutton_new(win, "mH", _("Inductance: "));
	widget_bind(fsu, "value", WIDGET_DOUBLE, &inductance);
	event_new(fsu, "fspinbutton-changed", impedance_update, NULL);
	
	fsu = fspinbutton_new(win, "ohms", _("Resistance: "));
	widget_bind(fsu, "value", WIDGET_DOUBLE, &resistance);
	event_new(fsu, "fspinbutton-changed", impedance_update, NULL);
}

