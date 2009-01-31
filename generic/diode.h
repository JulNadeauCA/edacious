/*	Public domain	*/

typedef struct es_diode {
	struct es_component _inherit;
	M_Real Is;			/* Reverse saturation current (A) */
	M_Real Vt;			/* Thermal voltage (V) */
	M_Real Ieq, g;			/* Companion model parameters */
	M_Real vPrevIter;
	StampConductanceData s_conductance;
	StampCurrentSourceData s_current_source;
} ES_Diode;

__BEGIN_DECLS
extern ES_ComponentClass esDiodeClass;
__END_DECLS
