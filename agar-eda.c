/*	$Csoft: morbgate.c,v 1.7 2005/05/25 07:03:54 vedge Exp $	*/

/*
 * Copyright (c) 2003, 2004, 2005 Winds Triton Engineering, Inc.
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
#include <engine/config.h>
#include <engine/view.h>
#include <engine/typesw.h>

#include <engine/map/map.h>
#include <engine/map/mapedit.h>

#include <engine/widget/window.h>
#include <engine/widget/tlist.h>
#include <engine/widget/fspinbutton.h>

#include <circuit/circuit.h>
#include "tool.h"

#include <string.h>
#include <unistd.h>

#include <component/conductor.h>
#include <component/vsource.h>
#include <component/resistor.h>
#include <component/semiresistor.h>
#include <component/inverter.h>
#include <component/spst.h>
#include <component/spdt.h>

extern const struct component_ops conductor_ops;
extern const struct component_ops vsource_ops;
extern const struct component_ops resistor_ops;
extern const struct component_ops semiresistor_ops;
extern const struct component_ops inverter_ops;
extern const struct component_ops spst_ops;
extern const struct component_ops spdt_ops;

const struct eda_type eda_models[] = {
	{ "conductor",	sizeof(struct conductor),	&conductor_ops },
	{ "vsource",	sizeof(struct vsource),		&vsource_ops },
	{ "resistor",	sizeof(struct resistor),	&resistor_ops },
	{ "semiresistor", sizeof(struct semiresistor),	&semiresistor_ops },
	{ "inverter",	sizeof(struct inverter),	&inverter_ops },
	{ "spst",	sizeof(struct spst),		&spst_ops },
	{ "spdt",	sizeof(struct spdt),		&spdt_ops },
	{ NULL }
};

#if 0
extern const struct eda_tool *tools[];
extern const int ntools;

static void
selected_tool(int argc, union evarg *argv)
{
	struct tlist_item *it = argv[1].p;

	window_show(it->p1);
}

static void
tools_window(void)
{
	struct window *win;
	struct tlist *tl;
	int i;

	win = window_new(0, "tools");
	window_set_caption(win, _("EDA Tools"));
	window_set_position(win, WINDOW_UPPER_RIGHT, 0);

	tl = tlist_new(win, 0);
	for (i = 0; i < ntools; i++) {
		tools[i]->init();
		tlist_insert_item(tl, NULL,
		    _(tools[i]->name), tools[i]->win);
	}
	event_new(tl, "tlist-dblclick", selected_tool, NULL);
	window_show(win);
}
#endif

struct fspinbutton *
bind_double(struct window *win, const char *unit, double *p,
    void (*func)(int argc, union evarg *argv), const char *text)
{
	struct fspinbutton *fsu;

	fsu = fspinbutton_new(win, unit, text);
	widget_bind(fsu, "value", WIDGET_DOUBLE, p);
	if (func != NULL) {
		event_new(fsu, "fspinbutton-changed", func, NULL);
	}
	return (fsu);
}

struct fspinbutton *
bind_double_ro(struct window *win, const char *unit, double *p,
    const char *text)
{
	struct fspinbutton *fsb;

	fsb = bind_double(win, unit, p, NULL, text);
	return (fsb);
}

int
main(int argc, char *argv[])
{
	extern const struct object_ops circuit_ops;
	const struct eda_type *ty;
	int c, i, fps = 17;
	char *s;

	if (engine_preinit("agar-eda") == -1) {
		fprintf(stderr, "%s\n", error_get());
		return (1);
	}

	while ((c = getopt(argc, argv, "?vt:r:T:t:gG")) != -1) {
		extern char *optarg;

		switch (c) {
		case 'v':
			exit(0);
		case 'r':
			view_parse_fpsspec(optarg);
			break;
		case 'T':
			prop_set_string(config, "font-path", "%s", optarg);
			break;
		case 't':
			text_parse_fontspec(optarg);
			break;
#ifdef HAVE_OPENGL
		case 'g':
			prop_set_bool(config, "view.opengl", 1);
			break;
		case 'G':
			prop_set_bool(config, "view.opengl", 0);
			break;
#endif
		case '?':
		default:
			printf("Usage: %s [-v] [-r fps] [-T font-path] "
			       "[-t fontspec]", progname);
#ifdef HAVE_OPENGL
			printf(" [-gG]");
#endif
			printf("\n");
			exit(0);
		}
	}
	if (engine_init() == -1) {
		fprintf(stderr, "%s\n", error_get());
		return (-1);
	}

	for (ty = &eda_models[0]; ty->name != NULL; ty++) {
		typesw_register(ty->name, ty->size, ty->ops,
		    EDA_COMPONENT_ICON);
	}
	typesw_register("circuit", sizeof(struct circuit), &circuit_ops,
	    EDA_CIRCUIT_ICON);

	mapedit_init();
	object_load(world);
#if 0
	tools_window();
#endif

	event_loop();
	engine_destroy();
	return (0);
}

