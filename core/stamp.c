/* Public domain */

/* Utility functions for stamping commonly used linear components */

#include <eda.h>

/* Dummy variable to write everything related to ground to */
M_Real dummy;

#define N SIM(dc)->ckt->n

/* to remove later, and move to M library */
#define M_GetElement(A, k, l) &(A->v[k][l])
#define M_GetElementVector(z, k) &(z->v[k][0])

#define GetElemG(k, l) (((k) == 0 || (l) == 0) ? &dummy : M_GetElement(dc->A, (k), (l)))
#define GetElemB(k, l) GetElemG(k, N+l)
#define GetElemC(k, l) GetElemG(N+k, l)
#define GetElemD(k, l) GetElemG(N+k, N+l)

#define GetElemI(k) ((k) == 0 ? &dummy : M_GetElementVector(dc->z, (k)))
#define GetElemV(k) GetElemI(k+N)


/* Conductance */
void InitStampConductance(Uint k, Uint l, StampData s, ES_SimDC *dc)
{
	s[0] = GetElemG(k, k);
	s[1] = GetElemG(k, l);
	s[2] = GetElemG(l, k);
	s[3] = GetElemG(l, l);
}
void
StampConductance(M_Real g, StampData s)
{
	*s[0] += g;
	*s[1] -= g;
	*s[2] -= g;
	*s[3] += g;
}


/* Current source */
void InitStampCurrentSource(Uint k, Uint l, StampData s, ES_SimDC *dc)
{
	s[0] = GetElemI(k);
	s[1] = GetElemI(l);
}
void
StampCurrentSource(M_Real I, StampData s)
{
	*s[0] += I;
	*s[1] -= I;
}


/* Voltage controlled current source */
void InitStampVCCS(Uint k, Uint l, Uint p, Uint q, StampData s, ES_SimDC *dc)
{
	s[0] = GetElemG(p, k);
	s[1] = GetElemG(p, l);
	s[2] = GetElemG(q, k);
	s[3] = GetElemG(q, l);
}
void
StampVCCS(M_Real g, StampData s)
{
	*s[0] += g;
	*s[1] -= g;
	*s[2] -= g;
	*s[3] += g;
}


/* Independant voltage source */
void InitStampVoltageSource(Uint k, Uint l, Uint v, StampData s, ES_SimDC *dc)
{
	s[0] = GetElemB(k, v);
	s[1] = GetElemC(v, k);
	s[2] = GetElemB(l, v);
	s[3] = GetElemC(v, l);
	s[4] = GetElemV(v);
}
void 
StampVoltageSource(M_Real voltage, StampData s)
{
	*s[0] = 1.0;
	*s[1] = 1.0;
	*s[2] = -1.0;
	*s[3] = -1.0;
	*s[4] = voltage;
}


/* Thevenin model */
void InitStampThevenin(Uint k, Uint l, Uint v, StampData s, ES_SimDC *dc)
{
	InitStampVoltageSource(k, l, v, s, dc);
	s[5] = GetElemD(v, v);
}
void
StampThevenin(M_Real voltage, M_Real resistance, StampData s)
{
	StampVoltageSource(voltage, s);
	*s[5] = -resistance;
}
