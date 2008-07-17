/*
 * Copyright (c) 2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * transient: Perform transient simulation on a circuit and output the
 * results at every timestep.
 */

#include <core/core.h>
#include <generic/generic.h>
#include <sources/sources.h>
#include <macro/macro.h>

volatile int doExit = 0;
int maxSteps = 0;
int curSteps = 0;
int showHeader = 1;
char **vars = NULL;
Uint nVars = 0;

static void
printusage(void)
{
	fprintf(stderr, "Usage: transient [-H] [-s maxSteps] [file] "
	                "[var1] [var2] [...]\n");
	exit(1);
}
		
static void
PrintHeader(void)
{
	int i;

	printf(" TIMESTEP ");
	for (i = 0; i < nVars; i++) {
		printf(" %11s", vars[i]);
	}
	printf("\n");
	printf(" -------- ");
	for (i = 0; i < nVars; i++) {
		printf(" -----------");
	}
	printf("\n");
}

static void
Step(AG_Event *event)
{
	ES_Circuit *ckt = AG_SELF();
	ES_SimDC *sim = AG_PTR(1);
	M_Real v;
	int i;

	printf("[%.06f] ", sim->Telapsed);
	if (nVars == 0) {
		printf("OK");
	}
	for (i = 0; i < nVars; i++) {
		v = M_GetReal(ckt, vars[i]);
		if (v > 0.0) { printf(" "); }
		printf("%.08f ", v);
	}
	printf("\n");

	if (maxSteps > 0 && ++curSteps >= maxSteps) {
		doExit = 1;
	}
}

int
main(int argc, char *argv[])
{
	ES_Circuit *ckt;
	ES_SimDC *sim;
	char *file;
	int i, c;

	AG_InitCore("transient", 0);
	agDebugLvl = 0;

	ES_CoreInit();
	ES_GenericInit();
	ES_MacroInit();
	ES_SourcesInit();
	
	while ((c = getopt(argc, argv, "?hs:H")) != -1) {
		extern char *optarg;

		switch (c) {
		case 's':
			maxSteps = atoi(optarg);
			break;
		case 'H':
			showHeader = 0;
			break;
		case '?':
		case 'h':
			printusage();
		}
	}
	if (optind == argc) {
		printusage();
	}
	nVars = argc-optind-1;
	vars = Malloc(nVars*sizeof(char *));
	for (i = 0; i < nVars; i++) {
		vars[i] = Strdup(argv[optind+1+i]);
	}

	file = argv[optind];
	printf("%s:\n", file);
	ckt = AG_ObjectNew(NULL, NULL, &esCircuitClass);
	if (AG_ObjectLoadFromFile(ckt, file) == -1) {
		fprintf(stderr, "%s: %s\n", file, AG_GetError());
		exit(1);
	}

	sim = (ES_SimDC *)ES_SetSimulationMode(ckt, &esSimDcOps);
	AG_SetEvent(ckt, "circuit-step-begin", Step, "%p", sim);
	SIM(sim)->ops->start(sim);

	if (showHeader) {
		PrintHeader();
	}
	for (;;) {
		if (doExit) {
			break;
		} else if (AG_TIMEOUTS_QUEUED()) {
			AG_ProcessTimeouts(SDL_GetTicks());
		}
	}
	AG_ObjectDestroy(ckt);
	return (0);
}
