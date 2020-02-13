/*
 * Copyright (c) 2008-2020 Julien Nadeau Carriere (vedge@csoft.net)
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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

volatile int doExit = 0;
int maxSteps = 0;
int curSteps = 0;
int showHeader = 1;
int plotDerivative = 0;

char fmtString[16];
char **vars = NULL;
M_Real *vPrev = NULL;
Uint nVars = 0;

static void
printusage(void)
{
	fprintf(stderr, "Usage: transient [-dHg] [-s maxSteps] [-p prec] "
	                "[file] [var1] [var2] [...]\n");
	exit(1);
}
		
static void
PrintHeader(void)
{
	int i;

	printf("#Time\t");

	for (i = 0; i < nVars; i++)
		if (strlen(vars[i]) > 0)
		{
			if (vars[i][0] == 'v')
			{
				vars[i]++;
				printf("v(%s)\t", vars[i]);
				vars[i]--;
			}
			else if (vars[i][0] == 'i')
			{
				vars[i]++;
				printf("i(%s)\t", vars[i]);
				vars[i]--;
			}
		}
	
	printf("\n");
}

static void
StepBegin(AG_Event *event)
{
	ES_Circuit *ckt = ES_CIRCUIT_PTR(1);
	ES_SimDC *sim = AG_PTR(2);
	M_Real v;
	int i;

	printf("%.06f\t", sim->Telapsed);
	if (nVars == 0) {
		printf("OK");
	}
	for (i = 0; i < nVars; i++) {
		v = M_GetReal(ckt, vars[i]);
		if (plotDerivative) {
			printf(fmtString, v-vPrev[i]);
			vPrev[i] = v;
		} else {
			printf("%.08f\t", v);
		}
	}
	printf("\n");

	if (maxSteps > 0 && ++curSteps >= maxSteps)
		doExit = 1;
}

int
main(int argc, char *argv[])
{
	ES_Circuit *ckt;
	ES_SimDC *sim;
	char *file;
	int i, c;
	AG_Object *mon;
	int prec = 8;
	char pfmt = 'f';

	AG_InitCore("transient", 0);
	ES_CoreInit(0);
	agDebugLvl = 0;

	while ((c = getopt(argc, argv, "?hHdgs:p:")) != -1) {
		extern char *optarg;

		switch (c) {
		case 'H':
			showHeader = 0;
			break;
		case 'd':
			plotDerivative = 1;
			break;
		case 'g':
			pfmt = 'g';
			break;
		case 's':
			maxSteps = atoi(optarg);
			break;
		case 'p':
			prec = atoi(optarg);
			break;
		case '?':
		case 'h':
			printusage();
		}
	}
	Snprintf(fmtString, sizeof(fmtString), "%%.0%d%c ",
	    prec, pfmt);

	if (optind == argc) {
		printusage();
	}
	nVars = argc-optind-1;
	vars = Malloc(nVars*sizeof(char *));
	vPrev = Malloc(nVars*sizeof(char *));
	for (i = 0; i < nVars; i++) {
		vars[i] = Strdup(argv[optind+1+i]);
		vPrev[i] = 0.0;
	}

	file = argv[optind];

	ckt = AG_ObjectNew(NULL, NULL, &esCircuitClass);
	if (AG_ObjectLoadFromFile(ckt, file) == -1) {
		fprintf(stderr, "%s: %s\n", file, AG_GetError());
		exit(1);
	}
	
	/* Initialize and begin transient simulation. */
	sim = (ES_SimDC *)ES_SetSimulationMode(ckt, &esSimDcOps);
	
	/* Create a "monitor" object to receive notification events. */
	mon = AG_ObjectNew(NULL, "mon", &agObjectClass);
	ES_AddSimulationObj(ckt, "Monitor", mon);
	AG_SetEvent(mon, "circuit-step-begin", StepBegin, "%p,%p", ckt, sim);

	if (showHeader)
		PrintHeader();

	/* Transient simulation loop */
	SIM(sim)->ops->start(sim);
	for (;;) {
		if (doExit) {
			break;
		} else if (!TAILQ_EMPTY(&agTimerObjQ)) {
			AG_ProcessTimeouts(AG_GetTicks());
		}
	}

	Free(vars);
	Free(vPrev);
	AG_ObjectDestroy(mon);
	AG_ObjectDestroy(ckt);
	return (0);
}
