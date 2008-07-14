/*	Public domain	*/

typedef struct es_nmos {
	struct es_component _inherit;

	M_Real Vt;			/* Threshold voltage */
	M_Real Va;			/* Early voltage */
	M_Real K;			/* K-value */

	M_Real Ieq, gm, go;		/* Companion model parameters */
	M_Real IeqPrev, gmPrev, goPrev;	

	M_Real *s_vccs[STAMP_VCCS_SIZE];
	M_Real *s_conductance[STAMP_CONDUCTANCE_SIZE];
	M_Real *s_current[STAMP_CURRENT_SOURCE_SIZE];
} ES_NMOS;

__BEGIN_DECLS
extern ES_ComponentClass esNMOSClass;
__END_DECLS
