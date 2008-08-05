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

/* smallest and largest acceptable conductances */
#define G_TINY 1e-9
#define G_HUGE 1e9

/* Macros to simplify function bodies */
#define SIM_N SIM(dc)->ckt->n

#define GetElemG(k, l) (((k) == 0 || (l) == 0) ? &esDummy : M_GetElement(dc->A, (k), (l)))
#define GetElemB(k, l) GetElemG(k, SIM_N+l)
#define GetElemC(k, l) GetElemG(SIM_N+k, l)
#define GetElemD(k, l) GetElemG(SIM_N+k, SIM_N+l)

#define GetElemI(k) ((k) == 0 ? &esDummy : M_VecGetElement(dc->z, (k)))
#define GetElemV(k) GetElemI(k+SIM_N)

__BEGIN_DECLS

extern M_Real esDummy;

/* Conductance */
typedef M_Real *StampConductanceData[4];

static void __inline__
InitStampConductance(Uint k, Uint l, StampConductanceData s, ES_SimDC *dc)
{
	s[0] = GetElemG(k, k);
	s[1] = GetElemG(k, l);
	s[2] = GetElemG(l, k);
	s[3] = GetElemG(l, l);
}
static void __inline__
StampConductance(M_Real g, StampConductanceData s)
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
typedef M_Real *StampCurrentSourceData[2];
static void __inline__
InitStampCurrentSource(Uint k, Uint l, StampCurrentSourceData s, ES_SimDC *dc)
{
	s[0] = GetElemI(k);
	s[1] = GetElemI(l);
}
static void __inline__
StampCurrentSource(M_Real I, StampCurrentSourceData s)
{
	*s[0] += I;
	*s[1] -= I;
}

/* Voltage controlled current source */
typedef M_Real *StampVCCSData[4];
static void __inline__
InitStampVCCS(Uint k, Uint l, Uint p, Uint q, StampVCCSData s, ES_SimDC *dc)
{
	s[0] = GetElemG(p, k);
	s[1] = GetElemG(p, l);
	s[2] = GetElemG(q, k);
	s[3] = GetElemG(q, l);
}
static void __inline__
StampVCCS(M_Real g, StampVCCSData s)
{
	*s[0] += g;
	*s[1] -= g;
	*s[2] -= g;
	*s[3] += g;
}

/* Independant voltage source */
typedef M_Real *StampVoltageSourceData[5];
static void __inline__
InitStampVoltageSource(Uint k, Uint l, Uint v, StampVoltageSourceData s, ES_SimDC *dc)
{
	s[0] = GetElemB(k, v);
	s[1] = GetElemC(v, k);
	s[2] = GetElemB(l, v);
	s[3] = GetElemC(v, l);
	s[4] = GetElemV(v);
}
static void __inline__ 
StampVoltageSource(M_Real voltage, StampVoltageSourceData s)
{
	*s[0] = 1.0;
	*s[1] = 1.0;
	*s[2] = -1.0;
	*s[3] = -1.0;
	*s[4] = voltage;
}

/* Thevenin model */
typedef M_Real *StampTheveninData[6];
static void __inline__
InitStampThevenin(Uint k, Uint l, Uint v, StampTheveninData s, ES_SimDC *dc)
{
	InitStampVoltageSource(k, l, v, s, dc);
	s[5] = GetElemD(v, v);
}
static void __inline__
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

#undef SIM_N
