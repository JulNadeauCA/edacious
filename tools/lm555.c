/*	$Csoft: lm555.c,v 1.5 2005/05/24 08:15:40 vedge Exp $	*/

/*
 * Copyright (c) 2004 Winds Triton Engineering, Inc.
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
#include <engine/widget/combo.h>

#include <math.h>

#include "../tool.h"

static void lm555_init(void);
static void update_ne555c(void);

struct eda_tool lm555_tool = {
	N_("LM555 Timer Adjustement Tool"),
	14,
	lm555_init,
	NULL
};

static const struct model {
	char	*name;
	void	(*update_freq)(void);
} models[] = {
	{ "NE/SA/SE555/SE555C",	update_ne555c },
	{ NULL,			NULL }
};

static double freq = 0;
static double R1 = 5000;
static double R2 = 5000;
static double C = 0.00000047;
static const struct model *model = &models[0];

static void
update_ne555c(void)
{
	freq = 1.49 / ((R1 + 2*R2) * C);
}

static void
update(int argc, union evarg *argv)
{
	model->update_freq();
}

static void
update_model(int argc, union evarg *argv)
{
	struct tlist_item *it = argv[1].p;

	model = it->p1;
	update(0, NULL);
}

static void
lm555_init(void)
{
	struct window *win;
	const struct model *mod;
	struct combo *com;

	win = lm555_tool.win = window_new(WINDOW_NO_VRESIZE, "lm555");
	window_set_caption(win, lm555_tool.name);
	
	com = combo_new(win, 0, _("Model: "));
	for (mod = &models[0]; mod->name != NULL; mod++) {
		tlist_insert_item(com->list, NULL, mod->name, (void *)mod);
	}
	event_new(com, "combo-selected", update_model, NULL);

	bind_double(win, "Hz", &freq, NULL, _("Frequency: "));
	bind_double(win, "ohms", &R1, update, "R1: ");
	bind_double(win, "ohms", &R2, update, "R2: ");
	bind_double(win, "uF", &C, update, "C: ");
}

