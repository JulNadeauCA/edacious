/*
 * Copyright (c) 2003-2009 Hypertriton, Inc. <http://hypertriton.com/>
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

/*
 * Main user interface code for Edacious.
 */

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>

#include <core/core.h>
#include <generic/generic.h>
#include <macro/macro.h>
#include <sources/sources.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <config/enable_nls.h>
#include <config/have_getopt.h>
#include <config/edacious_version.h>

static void
QuitByKBD(void)
{
	ES_GUI_Quit(NULL);
}

int
main(int argc, char *argv[])
{
	Uint coreFlags = ES_INIT_PRELOAD_ALL;
	int c, i;
	const char *driverSpec = NULL;

#ifdef ENABLE_NLS
	bindtextdomain("edacious", LOCALEDIR);
	bind_textdomain_codeset("edacious", "UTF-8");
	textdomain("edacious");
#endif
	if (AG_InitCore("edacious", AG_VERBOSE|AG_CREATE_DATADIR) == -1) {
		fprintf(stderr, "InitCore: %s\n", AG_GetError());
		return (1);
	}
#ifdef HAVE_GETOPT
	while ((c = getopt(argc, argv, "?vPd:t:T:")) != -1) {
		extern char *optarg;

		switch (c) {
		case 'v':
			printf("Edacious %s\n", EDACIOUS_VERSION);
			return (0);
		case 'P':
			Verbose("Not preloading modules\n");
			coreFlags &= ~(ES_INIT_PRELOAD_ALL);
			break;
		case 'd':
			driverSpec = optarg;
			break;
		case 'T':
			AG_SetString(agConfig, "font-path", optarg);
			break;
		case 't':
			AG_TextParseFontSpec(optarg);
			break;
		case '?':
		default:
			printf("Usage: %s [-vP] [-d agar-driver-spec] "
			       "[-t font-spec] [-T font-path]\n", agProgName);
			return (1);
		}
	}
#endif /* HAVE_GETOPT */

	if (AG_InitGraphics(driverSpec) == -1) {
		goto fail;
	}
	AG_BindGlobalKey(AG_KEY_ESCAPE, AG_KEYMOD_ANY, QuitByKBD);
	AG_BindGlobalKey(AG_KEY_F8, AG_KEYMOD_ANY, AG_ViewCapture);

	/*
	 * Initialize the Edacious library. Unless -P was given, we preload
	 * all modules at this point.
	 */
	ES_CoreInit(coreFlags);
	
	/* Create the application menu. */ 
	if (agDriverSw != NULL) {
		ES_InitMenuMDI();
	} else {
		AG_Object *objNew;
		if ((objNew = AG_ObjectNew(&esVfsRoot, NULL, &esCircuitClass))
		    == NULL) {
			goto fail;
		}
		if (ES_OpenObject(objNew) == NULL) {
			goto fail;
		}
	}

#ifdef HAVE_GETOPT
	for (i = optind; i < argc; i++) {
#else
	for (i = 1; i < argc; i++) {
#endif
		AG_Event ev;
		char *ext;

		Verbose("Loading: %s\n", argv[i]);
		if ((ext = strrchr(argv[i], '.')) == NULL)
			continue;

		AG_EventInit(&ev);
		if (strcasecmp(ext, ".ecm") == 0) {
			AG_EventPushPointer(&ev, "", &esCircuitClass);
		} else if (strcasecmp(ext, ".esh") == 0) {
			AG_EventPushPointer(&ev, "", &esSchemClass);
		} else if (strcasecmp(ext, ".ecl") == 0) {
			AG_EventPushPointer(&ev, "", &esLayoutClass);
		} else if (strcasecmp(ext, ".edp") == 0) {
			AG_EventPushPointer(&ev, "", &esPackageClass);
		} else if (strcasecmp(ext, ".em") == 0) {
			AG_EventPushPointer(&ev, "", &esComponentClass);
		} else {
			Verbose("Ignoring argument: %s\n", argv[i]);
			continue;
		}
		AG_EventPushString(&ev, "", argv[i]);
		ES_GUI_LoadObject(&ev);
	}

	AG_EventLoop();
	AG_ObjectDestroy(&esVfsRoot);
	AG_Destroy();
	return (0);
fail:
	fprintf(stderr, "%s\n", AG_GetError());
	AG_Destroy();
	return (1);
}
