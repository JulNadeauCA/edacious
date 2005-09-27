/*	$Csoft: aeda.c,v 1.5 2005/09/26 05:32:44 vedge Exp $	*/

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
#include <engine/config.h>
#include <engine/view.h>
#include <engine/typesw.h>

#include <engine/map/map.h>
#include <engine/map/mapedit.h>

#include <engine/widget/window.h>
#include <engine/widget/tlist.h>
#include <engine/widget/fspinbutton.h>

#include "aeda.h"

#include <circuit/circuit.h>

#include <string.h>
#include <unistd.h>

#include <component/conductor.h>
#include <component/vsource.h>
#include <component/ground.h>
#include <component/resistor.h>
#include <component/semiresistor.h>
#include <component/inverter.h>
#include <component/spst.h>
#include <component/spdt.h>

extern const struct component_ops conductor_ops;
extern const struct component_ops vsource_ops;
extern const struct component_ops ground_ops;
extern const struct component_ops resistor_ops;
extern const struct component_ops semiresistor_ops;
extern const struct component_ops inverter_ops;
extern const struct component_ops spst_ops;
extern const struct component_ops spdt_ops;

const struct eda_type eda_models[] = {
	{ "conductor",	sizeof(struct conductor),	&conductor_ops },
	{ "vsource",	sizeof(struct vsource),		&vsource_ops },
	{ "ground",	sizeof(struct ground),		&ground_ops },
	{ "resistor",	sizeof(struct resistor),	&resistor_ops },
	{ "semiresistor", sizeof(struct semiresistor),	&semiresistor_ops },
	{ "inverter",	sizeof(struct inverter),	&inverter_ops },
	{ "spst",	sizeof(struct spst),		&spst_ops },
	{ "spdt",	sizeof(struct spdt),		&spdt_ops },
	{ NULL }
};

int
main(int argc, char *argv[])
{
	extern const AG_ObjectOps circuit_ops;
	const struct eda_type *ty;
	int c, i, fps = 60;
	char *s;

	if (AG_InitCore("aeda", 0) == -1) {
		fprintf(stderr, "%s\n", AG_GetError());
		return (1);
	}

	while ((c = getopt(argc, argv, "?vt:r:T:t:gG")) != -1) {
		extern char *optarg;

		switch (c) {
		case 'v':
			exit(0);
		case 'r':
			fps = atoi(optarg);
			break;
		case 'T':
			AG_SetString(agConfig, "font-path", "%s", optarg);
			break;
		case 't':
			AG_TextParseFontSpec(optarg);
			break;
#ifdef HAVE_OPENGL
		case 'g':
			AG_SetBool(agConfig, "view.opengl", 1);
			break;
		case 'G':
			AG_SetBool(agConfig, "view.opengl", 0);
			break;
#endif
		case '?':
		default:
			printf("Usage: %s [-v] [-r fps] [-T font-path] "
			       "[-t fontspec]", agProgName);
#ifdef HAVE_OPENGL
			printf(" [-gG]");
#endif
			printf("\n");
			exit(0);
		}
	}
	if (AG_InitVideo(640, 480, 32, 0) == -1 ||
	    AG_InitInput(AG_INPUT_KBDMOUSE)) {
		fprintf(stderr, "%s\n", AG_GetError());
		return (-1);
	}
	AG_InitConfigWin(AG_CONFIG_ALL);
	AG_SetRefreshRate(fps);

	for (ty = &eda_models[0]; ty->name != NULL; ty++) {
		char tname[AG_OBJECT_NAME_MAX];
	
		strlcpy(tname, "component.", sizeof(tname));
		strlcat(tname, ty->name, sizeof(tname));
		AG_RegisterType(tname, ty->size, ty->ops, EDA_COMPONENT_ICON);
	}
	AG_RegisterType("circuit", sizeof(struct circuit), &circuit_ops,
	    EDA_CIRCUIT_ICON);

	AG_MapEditorInit();
	AG_ObjectLoad(agWorld);
	AG_EventLoop();
	AG_Quit();
	return (0);
}

