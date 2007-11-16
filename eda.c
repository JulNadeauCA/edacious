/*
 * Copyright (c) 2003-2005 Hypertriton, Inc. <http://hypertriton.com/>
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
#include <agar/dev.h>

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

extern const ES_ComponentClass esVsourceClass;
extern const ES_ComponentClass esGroundClass;
extern const ES_ComponentClass esResistorClass;
extern const ES_ComponentClass esSemiResistorClass;
extern const ES_ComponentClass esInverterClass;
extern const ES_ComponentClass esAndClass;
extern const ES_ComponentClass esOrClass;
extern const ES_ComponentClass esSpstClass;
extern const ES_ComponentClass esSpdtClass;
extern const ES_ComponentClass esLedClass;
extern const ES_ComponentClass esLogicProbeClass;
extern const ES_ComponentClass esVSquareClass;
extern const ES_ComponentClass esVSineClass;

const void *eda_models[] = {
	&esVsourceClass,
	&esGroundClass,
	&esResistorClass,
	&esSemiResistorClass,
	&esSpstClass,
	&esSpdtClass,
	&esLedClass,
	&esLogicProbeClass,
	&esInverterClass,
	&esAndClass,
	&esOrClass,
	&esVSquareClass,
	&esVSineClass,
	NULL
};

int
main(int argc, char *argv[])
{
	extern const AG_ObjectClass esCircuitClass;
	extern const AG_ObjectClass esScopeClass;
	const void **model;
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
	    == -1) {
		fprintf(stderr, "%s\n", AG_GetError());
		return (-1);
	}
	AG_InitInput(0);
	VG_InitSubsystem();
	AG_SetRefreshRate(fps);
	AG_BindGlobalKey(SDLK_ESCAPE, KMOD_NONE, AG_Quit);
	AG_BindGlobalKey(SDLK_F8, KMOD_NONE, AG_ViewCapture);
	
	AG_RegisterClass(&esComponentClass);
	AG_RegisterClass(&esDigitalClass);
	for (model = &eda_models[0]; *model != NULL; model++) {
		AG_RegisterClass(*model);
	}
	AG_RegisterClass(&esCircuitClass);
	AG_RegisterClass(&esScopeClass);
	
	if (AG_ObjectLoad(agWorld) == -1) {
		AG_ObjectSave(agWorld);		/* Assume initial run */
	}
	
	/* Initialize the object manager. */
	DEV_InitSubsystem(0);
	DEV_Browser();

	AG_EventLoop();
	AG_Quit();
	return (0);
}

