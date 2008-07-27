/*	Public domain	*/

typedef struct es_pmos {
	struct es_component _inherit;
	
	M_Real Vt;		/* Threshold voltage */
	M_Real Va;		/* Early voltage */
	M_Real K;		/* K-value */

	M_Real Ieq;
	M_Real gm;
	M_Real go;
	
	StampVCCSData s_vccs;
	StampConductanceData s_conductance;
	StampCurrentSourceData s_current;
} ES_PMOS;

__BEGIN_DECLS
extern ES_ComponentClass esPMOSClass;
__END_DECLS
