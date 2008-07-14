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

int
main(int argc, char *argv[])
{
	ES_Circuit *ckt;
	char *file;
	int i;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s [file]\n", argv[0]);
		return (1);
	}
	file = argv[1];

	AG_InitCore("ecminfo", 0);
	ES_CoreInit();
	ES_GenericInit();
	ES_MacroInit();
	ES_SourcesInit();

	ckt = AG_ObjectNew(&esVfsRoot, NULL, &esCircuitClass);
	if (AG_ObjectLoadFromFile(ckt, argv[1]) == 0) {
		printf("%s:\n", file);
		printf("Name: %s\n", AGOBJECT(ckt)->name);
		printf("Description: \"%s\"\n", ckt->descr);
		printf("Author: \"%s\"\n", ckt->authors);
		printf("Keywords: \"%s\"\n", ckt->keywords);
		printf("\n");
		printf("Total voltage sources: %u\n", ckt->m);
		printf("Total nodes: %u\n", ckt->n);
	} else {
		fprintf(stderr, "%s: %s\n", argv[1], AG_GetError());
	}
	return (0);
}
