/*
 * Copyright (c) 2008 
 *
 * Antoine Levitt (smeuuh@gmail.com)
 * Steven Herbst (herbst@mit.edu)
 *
 * Hypertriton, Inc. <http://hypertriton.com/>
 *
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

struct es_circuit;
struct ag_console;

typedef struct es_sim_ops {
	char *name;
	AG_StaticIcon *icon;
	size_t size;
	void (*init)(void *);
	void (*destroy)(void *);
	void (*start)(void *);
	void (*stop)(void *);
	void (*cktmod)(void *, struct es_circuit *);
	M_Real (*node_voltage)(void *, int);
	M_Real (*node_voltage_prev_step)(void *, int, int);
	M_Real (*branch_current)(void *, int);
	M_Real (*branch_current_prev_step)(void *, int, int);
	AG_Window *(*edit)(void *, struct es_circuit *);
} ES_SimOps;

typedef struct es_sim {
	struct es_circuit *ckt;		/* Circuit being analyzed */
	const ES_SimOps *ops;		/* Generic operations */
	AG_Window *win;			/* Settings window */
	int running;			/* Continous simulation in process */
} ES_Sim;

#define SIM(p) ((ES_Sim *)p)

__BEGIN_DECLS
void ES_SimInit(void *, const ES_SimOps *);
void ES_SimDestroy(void *);
void ES_SimEdit(AG_Event *);
void ES_SimLog(void *, const char *, ...);
__END_DECLS
