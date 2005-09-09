/*	$Csoft: avreg.c,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/

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
#include <engine/widget/combo.h>

#include <math.h>

#include "../tool.h"

static void avreg_init(void);
static void update_lm317(void);
static void update_lm337(void);

struct eda_tool avreg_tool = {
	N_("Adjustable Voltage Regulator Tool"),
	12,
	avreg_init,
	NULL
};

static const struct model {
	char	*name;
	void	(*update_fn)(void);
} models[] = {
	{ "LM317 Positive Adjustable Voltage Regulator",	update_lm317 },
	{ "LM337 Negative Adjustable Voltage Regulator",	update_lm337 },
	{ NULL,							NULL }
};

static double vo = 0;
static double r1 = 1, r2 = 1;
static double iadj = 1;
static const struct model *model = &models[0];

static void
update_lm317(void)
{
	vo = 1.25 * (1 + r2 / r1) + iadj * r2;
}

static void
update_lm337(void)
{
	vo = -1.25 * (1 + r2 / r1);
}

static void
update_vo(int argc, union evarg *argv)
{
	model->update_fn();
}

static void
update_model(int argc, union evarg *argv)
{
	struct tlist_item *it = argv[1].p;

	model = it->p1;
	update_vo(0, NULL);
}

static void
avreg_init(void)
{
	struct window *win;
	const struct model *mod;
	struct combo *com;

	win = avreg_tool.win = window_new(WINDOW_NO_VRESIZE, "avreg");
	window_set_caption(win, avreg_tool.name);
	
	com = combo_new(win, 0, _("Model: "));
	for (mod = &models[0]; mod->name != NULL; mod++) {
		tlist_insert_item(com->list, NULL, mod->name, (void *)mod);
	}
	event_new(com, "combo-selected", update_model, NULL);

	bind_double(win, "V", &vo, NULL, "Vo: ");
	bind_double(win, "ohms", &r1, update_vo, "R1: ");
	bind_double(win, "ohms", &r2, update_vo, "R2: ");
	bind_double(win, "mA", &iadj, update_vo, "Iadj: ");
}

