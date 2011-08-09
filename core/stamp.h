/*
 * Copyright (c) 2008 Antoine Levitt (smeuuh@gmail.com)
 * Copyright (c) 2008 Steven Herbst (herbst@mit.edu)
 * Copyright (c) 2008-2009 Julien Nadeau (vedge@hypertriton.com)
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
 * Utility functions for stamping commonly used linear components.
 *
 * Each element is composed of three symbols : its stamp size, an init function,
 * and a stamp function. The caller component must reserve a memory block of
 * stamp size M_Real elements, and pass it to the init function for it to
 * write the adress where subsequent writes will be done. Then the component
 * passes the StampData and the values to write each time it wants to stamp.
 */

/* Smallest and largest acceptable conductances */
#define G_TINY 1e-6
#define G_HUGE 1e6

/* Macros to simplify function bodies */
#define GetElemG(k, l) (((k) == 0 || (l) == 0) ? &esDummy : \
                       M_GetElement(dc->A, (k), (l)))
#define GetElemB(k, l) GetElemG(k, SIM(dc)->ckt->n + l)
#define GetElemC(k, l) GetElemG(SIM(dc)->ckt->n + k, l)
#define GetElemD(k, l) GetElemG(SIM(dc)->ckt->n+k, SIM(dc)->ckt->n+l)
#define GetElemI(k) ((k) == 0 ? &esDummy : M_VecGetElement(dc->z, (k)))
#define GetElemV(k) GetElemI(k+SIM(dc)->ckt->n)

typedef M_Real *StampConductanceData[4];
typedef M_Real *StampCurrentSourceData[2];
typedef M_Real *StampVCCSData[4];
typedef M_Real *StampVoltageSourceData[5];
typedef M_Real *StampTheveninData[6];

__BEGIN_DECLS

/* A dummy variable that will contain all stamps relating to the ground. */
extern M_Real esDummy;

/*
 * Conductance
 */
static __inline__ void
InitStampConductance(Uint k, Uint l, StampConductanceData s, ES_SimDC *dc)
{
	s[0] = GetElemG(k, k);
	s[1] = GetElemG(k, l);
	s[2] = GetElemG(l, k);
	s[3] = GetElemG(l, l);
}
static __inline__ void
StampConductance(M_Real g, StampConductanceData s)
{
	if (g < G_TINY) {
		g = G_TINY;
	} else if (g > G_HUGE) {
		g = G_HUGE;
	}
	*s[0] += g;
	*s[1] -= g;
	*s[2] -= g;
	*s[3] += g;
}

/*
 * Current source
 */
static __inline__ void
InitStampCurrentSource(Uint k, Uint l, StampCurrentSourceData s, ES_SimDC *dc)
{
	s[0] = GetElemI(k);
	s[1] = GetElemI(l);
}
static __inline__ void
StampCurrentSource(M_Real I, StampCurrentSourceData s)
{
	*s[0] += I;
	*s[1] -= I;
}

/*
 * Voltage controlled current source
 */
static __inline__ void
InitStampVCCS(Uint k, Uint l, Uint p, Uint q, StampVCCSData s, ES_SimDC *dc)
{
	s[0] = GetElemG(p, k);
	s[1] = GetElemG(p, l);
	s[2] = GetElemG(q, k);
	s[3] = GetElemG(q, l);
}
static __inline__ void
StampVCCS(M_Real g, StampVCCSData s)
{
	*s[0] += g;
	*s[1] -= g;
	*s[2] -= g;
	*s[3] += g;
}

/*
 * Independent voltage source
 */
static __inline__ void
InitStampVoltageSource(Uint k, Uint l, Uint v, StampVoltageSourceData s, ES_SimDC *dc)
{
	s[0] = GetElemB(k, v);
	s[1] = GetElemC(v, k);
	s[2] = GetElemB(l, v);
	s[3] = GetElemC(v, l);
	s[4] = GetElemV(v);
}
static __inline__ void
StampVoltageSource(M_Real voltage, StampVoltageSourceData s)
{
	*s[0] = 1.0;
	*s[1] = 1.0;
	*s[2] = -1.0;
	*s[3] = -1.0;
	*s[4] = voltage;
}

/*
 * Thevenin model
 */
static __inline__ void
InitStampThevenin(Uint k, Uint l, Uint v, StampTheveninData s, ES_SimDC *dc)
{
	InitStampVoltageSource(k, l, v, s, dc);
	s[5] = GetElemD(v, v);
}
static __inline__ void
StampThevenin(M_Real voltage, M_Real resistance, StampTheveninData s)
{
	if (resistance < (1/G_HUGE))
		resistance = 1/G_HUGE;
	else if (resistance > (1/G_TINY))
		resistance = 1/G_TINY;

	StampVoltageSource(voltage, s);
	*s[5] = -resistance;
}
__END_DECLS
