/*	Public domain	*/

typedef struct es_inductor {
	struct es_component _inherit;
	M_Real L;			/* Inductance (H) */
	M_Real g, Ieq;			/* Companion model parameters */
	M_Real *I;                      /* Past and present currents */
	StampConductanceData s_conductance;
	StampCurrentSourceData s_current_source;
} ES_Inductor;

__BEGIN_DECLS
extern ES_ComponentClass esInductorClass;
__END_DECLS
