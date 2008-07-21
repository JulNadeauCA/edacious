/*	Public domain	*/

typedef struct es_pmos {
	struct es_component _inherit;
	
	M_Real Vt;		/* Threshold voltage */
	M_Real Va;		/* Early voltage */
	M_Real K;		/* K-value */

	M_Real Ieq;
	M_Real gm;
	M_Real go;
	
	M_Real *s_vccs[STAMP_VCCS_SIZE];
	M_Real *s_conductance[STAMP_CONDUCTANCE_SIZE];
	M_Real *s_current[STAMP_CURRENT_SOURCE_SIZE];
} ES_PMOS;

__BEGIN_DECLS
extern ES_ComponentClass esPMOSClass;
__END_DECLS
