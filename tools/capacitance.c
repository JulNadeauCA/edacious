/*	$Csoft: capacitance.c,v 1.6 2005/05/24 08:15:40 vedge Exp $	*/

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
#include <engine/widget/combo.h>

#include <math.h>

#include "../tool.h"

static void capacitance_init(void);

struct eda_tool capacitance_tool = {
	N_("Material capacitance"),
	5,
	capacitance_init,
	NULL
};

static const struct dielectric {
	char	*name;
	double	 min, max;
} dielectric_constants[] = {
	{ N_("Vacuum"),			1,	1 },
	{ N_("Air (1 atm)"),		1.00059, 1.00059 },
	{ N_("Air (100 atm)"),		1.0548, 1.0548 },
	{ N_("Glass"),			1,	1 },
	{ N_("Mica"),			3,	6 },
	{ N_("Mylar"),			3.1,	3.1 },
	{ N_("Neoprene"),		6.70,	6.70 },
	{ N_("Plexiglas"),		3.40,	3.40 },
	{ N_("Polyethylene"),		2.25,	2.25 },
	{ N_("Polyvinyl chloride"),	3.18,	3.18 },
	{ N_("Teflon"),			2.1,	2.1 },
	{ N_("Germanium"),		16,	16 },
	{ N_("Strontium titanate"),	310,	310 },
	{ N_("Water"),			80.4,	80.4 },
	{ N_("Glycerin"),		42.5,	42.5 },
	{ N_("Benzene"),		2.284,	2.284 },
	{ N_("Silicium"),		12,	12 },
	{ "InSb",			18,	18 },
	{ "InAs",			14.5,	14.5 },
	{ "InP",			14,	14 },
	{ "GaSb",			15,	15 },
	{ "GaAs",			13,	13 },
	{ NULL,				0,	0 }
};

static double capacitance_min, capacitance_max;
static double area = 10e-6;
static double thick = 10e-6;
static double temp = 300.15;
static const struct dielectric *dielectric = &dielectric_constants[0];

static void
update(int argc, union evarg *argv)
{
	capacitance_min = (0.0885 * dielectric->min * area) / thick;
	capacitance_max = (0.0885 * dielectric->max * area) / thick;
}

static void
update_dielectric(int argc, union evarg *argv)
{
	struct tlist_item *it = argv[1].p;

	dielectric = it->p1;
	update(0, NULL);
}

static void
capacitance_init(void)
{
	struct window *win;
	struct fspinbutton *fsu;
	struct box *bo;
	struct combo *com;
	const struct dielectric *di;

	win = capacitance_tool.win = window_new(WINDOW_NO_VRESIZE, "capacitance");
	window_set_caption(win, _(capacitance_tool.name));

	bo = box_new(win, BOX_HORIZ, BOX_WFILL);
	{
		bind_double_ro(win, "uF", &capacitance_min, _("Min: "));
		bind_double_ro(win, "uF", &capacitance_max, _("Max: "));
	}

	com = combo_new(win, 0, _("Dielectric: "));
	for (di = &dielectric_constants[0]; di->name != NULL; di++) {
		tlist_insert_item(com->list, NULL, _(di->name), (void *)di);
	}
	textbox_printf(com->tbox, _("Vacuum"));
	event_new(com, "combo-selected", update_dielectric, NULL);

	bind_double(win, "um^2", &area, update, _("Area: "));
	bind_double(win, "um", &thick, update, _("Thickness: "));
	bind_double(win, "degC", &temp, update, _("Temperature: "));
}

