/*	Public domain	*/

typedef struct es_diode {
	struct es_component _inherit;

	M_Real Is;		/* Resistance at Tnom */
	M_Real Vt;		/* Thermal voltage */
	M_Real Ieq, g;		/* Companion model parameters */

	M_Real vPrevIter;

	M_Real *s_conductance[STAMP_CONDUCTANCE_SIZE];
	M_Real *s_current_source[STAMP_CONDUCTANCE_SIZE];
} ES_Diode;

__BEGIN_DECLS
extern ES_ComponentClass esDiodeClass;
__END_DECLS
