/*	$Csoft$	*/

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

#include <agar/core.h>
#include <agar/vg.h>
#include <agar/gui.h>

#include "eda.h"

#include <string.h>
#include <unistd.h>

#include <circuit/circuit.h>
#include <circuit/scope.h>

#include <component/resistor.h>
#include <component/semiresistor.h>
#include <component/inverter.h>
#include <component/and.h>
#include <component/or.h>
#include <component/spst.h>
#include <component/spdt.h>
#include <component/led.h>
#include <component/logic_probe.h>

#include <sources/vsquare.h>
#include <sources/vsine.h>

extern const ES_ComponentOps esVsourceOps;
extern const ES_ComponentOps esGroundOps;
extern const ES_ComponentOps esResistorOps;
extern const ES_ComponentOps esSemiResistorOps;
extern const ES_ComponentOps esInverterOps;
extern const ES_ComponentOps esAndOps;
extern const ES_ComponentOps esOrOps;
extern const ES_ComponentOps esSpstOps;
extern const ES_ComponentOps esSpdtOps;
extern const ES_ComponentOps esLedOps;
extern const ES_ComponentOps esLogicProbeOps;
extern const ES_ComponentOps esVSquareOps;
extern const ES_ComponentOps esVSineOps;

const struct eda_type eda_models[] = {
	{ "vsource",	sizeof(ES_Vsource),		&esVsourceOps },
	{ "ground",	sizeof(ES_Ground),		&esGroundOps },
	{ "resistor",	sizeof(ES_Resistor),		&esResistorOps },
	{ "semiresistor", sizeof(ES_SemiResistor),	&esSemiResistorOps },
	{ "spst",	sizeof(ES_Spst),		&esSpstOps },
	{ "spdt",	sizeof(ES_Spdt),		&esSpdtOps },
	{ "led",	sizeof(ES_Led),			&esLedOps },
	{ "logic_probe", sizeof(ES_LogicProbe),		&esLogicProbeOps },
	{ "digital.inverter", sizeof(ES_Inverter),	&esInverterOps },
	{ "digital.and", sizeof(ES_And),		&esAndOps },
	{ "digital.or",	sizeof(ES_Or),			&esOrOps },
	{ "vsource.square", sizeof(ES_VSquare),		&esVSquareOps },
	{ "vsource.sine", sizeof(ES_VSine),		&esVSineOps },
	{ NULL }
};

int
main(int argc, char *argv[])
{
	extern const AG_ObjectOps esCircuitOps;
	extern const AG_ObjectOps esScopeOps;
	const struct eda_type *ty;
	int c, i, fps = -1;
	char *s;

	if (AG_InitCore("agar-eda", 0) == -1) {
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
	if (AG_InitVideo(900, 600, 32, AG_VIDEO_RESIZABLE|AG_VIDEO_BGPOPUPMENU)
	    == -1 || AG_InitInput(0)) {
		fprintf(stderr, "%s\n", AG_GetError());
		return (-1);
	}
	AG_InitConfigWin(AG_CONFIG_ALL);
	AG_SetRefreshRate(fps);
	AG_BindGlobalKey(SDLK_ESCAPE, KMOD_NONE, AG_Quit);
	AG_BindGlobalKey(SDLK_F1, KMOD_NONE, AG_ShowSettings);
	AG_BindGlobalKey(SDLK_F8, KMOD_NONE, AG_ViewCapture);
	
	for (ty = &eda_models[0]; ty->name != NULL; ty++) {
		char tname[AG_OBJECT_NAME_MAX];
	
		strlcpy(tname, "component.", sizeof(tname));
		strlcat(tname, ty->name, sizeof(tname));
		AG_RegisterType(tname, ty->size, ty->ops, EDA_COMPONENT_ICON);
	}
	AG_RegisterType("circuit", sizeof(ES_Circuit), &esCircuitOps,
	    EDA_CIRCUIT_ICON);
	AG_RegisterType("scope", sizeof(ES_Scope), &esScopeOps,
	    EDA_CIRCUIT_ICON);
	
	/* Initialize the object manager. */
	AG_ObjMgrInit();
	AG_WindowShow(AG_ObjMgrWindow());

	AG_ObjectLoad(agWorld);
	AG_EventLoop();
	AG_Quit();
	return (0);
}

