/*	Public domain	*/

/*
 * Utility functions for stamping commonly used linear components.
 *
 * Each element is composed of three symbols : its stamp size, an init function,
 * and a stamp function. The caller component must reserve a memory block of
 * stamp size M_Real elements, and pass it to the init function for it to
 * write the adress where subsequent writes will be done. Then the component
 * passes the StampData and the values to write each time it wants to stamp.
 */


typedef M_Real *StampData[];

/* Dummy variable to write everything related to ground to */
extern M_Real dummy;


/* smallest and largest acceptable conductances */
#define G_TINY 1e-6
#define G_HUGE 1e6

/* Macros to simplify function bodies */
#define N SIM(dc)->ckt->n

#define GetElemG(k, l) (((k) == 0 || (l) == 0) ? &dummy : M_GetElement(dc->A, (k), (l)))
#define GetElemB(k, l) GetElemG(k, N+l)
#define GetElemC(k, l) GetElemG(N+k, l)
#define GetElemD(k, l) GetElemG(N+k, N+l)

#define GetElemI(k) ((k) == 0 ? &dummy : M_VecGetElement(dc->z, (k)))
#define GetElemV(k) GetElemI(k+N)

/* Conductance */
#define STAMP_CONDUCTANCE_SIZE 4
static void __inline__
InitStampConductance(Uint k, Uint l, StampData s, ES_SimDC *dc)
{
	s[0] = GetElemG(k, k);
	s[1] = GetElemG(k, l);
	s[2] = GetElemG(l, k);
	s[3] = GetElemG(l, l);
}
static void __inline__
StampConductance(M_Real g, StampData s)
{
	if (g < G_TINY)
		g = G_TINY;
	else if (g > G_HUGE)
		g = G_HUGE;

	*s[0] += g;
	*s[1] -= g;
	*s[2] -= g;
	*s[3] += g;
}

/* Current source */
#define STAMP_CURRENT_SOURCE_SIZE 2
static void __inline__
InitStampCurrentSource(Uint k, Uint l, StampData s, ES_SimDC *dc)
{
	s[0] = GetElemI(k);
	s[1] = GetElemI(l);
}
static void __inline__
StampCurrentSource(M_Real I, StampData s)
{
	*s[0] += I;
	*s[1] -= I;
}

/* Voltage controlled current source */
#define STAMP_VCCS_SIZE 4
static void __inline__
InitStampVCCS(Uint k, Uint l, Uint p, Uint q, StampData s, ES_SimDC *dc)
{
	s[0] = GetElemG(p, k);
	s[1] = GetElemG(p, l);
	s[2] = GetElemG(q, k);
	s[3] = GetElemG(q, l);
}
static void __inline__
StampVCCS(M_Real g, StampData s)
{
	*s[0] += g;
	*s[1] -= g;
	*s[2] -= g;
	*s[3] += g;
}

/* Independant voltage source */
#define STAMP_VOLTAGE_SOURCE_SIZE 5
static void __inline__
InitStampVoltageSource(Uint k, Uint l, Uint v, StampData s, ES_SimDC *dc)
{
	s[0] = GetElemB(k, v);
	s[1] = GetElemC(v, k);
	s[2] = GetElemB(l, v);
	s[3] = GetElemC(v, l);
	s[4] = GetElemV(v);
}
static void __inline__ 
StampVoltageSource(M_Real voltage, StampData s)
{
	*s[0] = 1.0;
	*s[1] = 1.0;
	*s[2] = -1.0;
	*s[3] = -1.0;
	*s[4] = voltage;
}

/* Thevenin model */
#define STAMP_THEVENIN_SIZE 6
static void __inline__
InitStampThevenin(Uint k, Uint l, Uint v, StampData s, ES_SimDC *dc)
{
	InitStampVoltageSource(k, l, v, s, dc);
	s[5] = GetElemD(v, v);
}
static void __inline__
StampThevenin(M_Real voltage, M_Real resistance, StampData s)
{
	if (resistance < (1/G_HUGE))
		resistance = 1/G_HUGE;
	else if (resistance > (1/G_TINY))
		resistance = 1/G_TINY;

	StampVoltageSource(voltage, s);
	*s[5] = -resistance;
}

#undef N
