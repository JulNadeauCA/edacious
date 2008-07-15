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
 * ecminfo: Output information pertaining to Edacious circuit model (.ecm)
 * files.
 */

#include <edacious/core.h>

int showProps = 0;

static void
printusage(void)
{
	fprintf(stderr, "Usage: ecminfo [-p] [file]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	ES_Circuit *ckt;
	int c;
	int i;

	AG_InitCore("ecminfo", 0);
	agDebugLvl = 0;

	ES_CoreInit();
	ES_GenericInit();
	ES_MacroInit();
	ES_SourcesInit();
	
	while ((c = getopt(argc, argv, "?hp")) != -1) {
		extern char *optarg;

		switch (c) {
		case 'p':
			showProps = 1;
			break;
		case '?':
		case 'h':
			printusage();
		}
	}
	for (i = optind; i < argc; i++) {
		printf("%s:\n", argv[i]);
		ckt = AG_ObjectNew(&esVfsRoot, NULL, &esCircuitClass);
		if (AG_ObjectLoadFromFile(ckt, argv[i]) == -1) {
			fprintf(stderr, "%s: %s\n", argv[i], AG_GetError());
			AG_ObjectDestroy(ckt);
			continue;
		}
		if (ckt->descr[0] != '\0') {
			printf("\tDescription: \"%s\"\n", ckt->descr);
		}
		if (ckt->authors[0] != '\0') {
			printf("\tAuthor: \"%s\"\n", ckt->authors);
		}
		if (ckt->keywords[0] != '\0') {
			printf("\tKeywords: \"%s\"\n", ckt->keywords);
		}
		printf("\tTotal voltage sources: %u\n", ckt->m);
		printf("\tTotal nodes: %u\n", ckt->n);

		if (showProps) {
			AG_Prop *prop;
			char s[1024];

			printf("\tProperties:\n");
			AGOBJECT_FOREACH_PROP(prop, ckt) {
				AG_PropPrint(s, sizeof(s), ckt, prop->key);
				printf("\t\t%s = %s\n", prop->key, s);
			}
		}
		if (!TAILQ_EMPTY(&AGOBJECT(ckt)->children)) {
			ES_Component *com;

			printf("\tComponents:\n");
			ESCIRCUIT_FOREACH_COMPONENT(com, ckt) {
				printf("%35s: %s (%up)",
				    ESCOMPONENT_CLASS(com)->name,
				    AGOBJECT(com)->name,
				    com->nports);
				if (com->flags & ES_COMPONENT_FLOATING) {
					printf(" <FLOATING>");
				}
				if (com->flags & ES_COMPONENT_SUPPRESSED) {
					printf(" <SUPPRESSED>");
				}
				printf("\n");
			}
		}
//		AG_ObjectDestroy(ckt);
	}
	return (0);
}
