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

typedef struct es_npn {
	struct es_component _inherit;

	M_Real Vt;		/* Thermal voltage */
	M_Real Va;		/* Early voltage */
	M_Real Ifs,Irs;
	M_Real betaF,betaR;		

	M_Real Ibf_eq;
	M_Real Ibr_eq;
	M_Real Icc_eq;
	M_Real gPiF;
	M_Real gmF;
	M_Real gPiR;
	M_Real gmR;
	M_Real go;

	M_Real VbePrevIter;
	M_Real VbcPrevIter;

	StampConductanceData sc_be;
	StampConductanceData sc_bc;
	StampConductanceData sc_ec;

	StampVCCSData sv_bece;
	StampVCCSData sv_bcec;
	
	StampCurrentSourceData si_eb;
	StampCurrentSourceData si_cb;
	StampCurrentSourceData si_ec;
} ES_NPN;

__BEGIN_DECLS
extern ES_ComponentClass esNPNClass;
__END_DECLS
