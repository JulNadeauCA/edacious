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

typedef struct es_sim_dc {
	struct es_sim _inherit;

	enum es_integration_method method;	/* Method of integration used */
	
	AG_Timeout toUpdate;	/* Timer for simulation updates */
	M_Real Telapsed;        /* Simulated elapsed time (s) */
	M_Real deltaT;          /* Simulated time since last iteration (s) */
	Uint   ticksDelay;	/* Simulation speed (delay ms) */
	Uint currStep;          /* Number of current step */
	
	Uint isDamped;		/* 1 if any components had to damp voltage
	                           guesses in the previous iteration, 0
				   otherwise */
	Uint inputStep;		/* flag that can be set by active components
	                           to aid simulation: 1 if there is a
				   discontinuity in input, 0 otherwise */
	Uint retriesMax;	/* Maximum number of times time step may be
				   decimated before halting simulation */

	Uint itersMax;		/* Limit on iterations/step */
	Uint itersHigh;		/* Most iterations/step recorded */
	Uint itersLow;		/* Least iterations/step recorded */
	M_Real stepLow;		/* Smallest timestep used */
	M_Real stepHigh;	/* Largest timestep used */

	M_Real T0;		/* Reference temperature */
	M_Matrix *A;		/* Block matrix [G,B; C,D] */
	M_Vector *z;		/* Right-hand side vector (i,e) */
	M_Vector *x;		/* Vector of unknowns (v,j) */

	M_Vector *xPrevIter;	/* Solution from last iteration */

	int stepsToKeep;        /* Number of previous solutions to keep */
	M_Vector **xPrevSteps;	/* Solutions from last steps */
	M_Real *deltaTPrevSteps;/* Previous timesteps. deltaTPrevSteps[i] is
				 * the timestep used to compute xPrevSteps[i] */

	M_Real *groundNode;     /* Pointer to A(0, 0) */
} ES_SimDC;

__BEGIN_DECLS
const ES_SimOps esSimDcOps;
__END_DECLS
