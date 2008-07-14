/*	Public domain	*/

typedef struct es_inductor {
	struct es_component _inherit;

	M_Real L;			/* Inductance (H) */
	M_Real g, Ieq;			/* Companion model parameters */
	M_Real gPrev, IeqPrev;

	M_Real *s_conductance[STAMP_CONDUCTANCE_SIZE];
	M_Real *s_current_source[STAMP_CURRENT_SOURCE_SIZE];
} ES_Inductor;

__BEGIN_DECLS
extern ES_ComponentClass esInductorClass;
__END_DECLS
