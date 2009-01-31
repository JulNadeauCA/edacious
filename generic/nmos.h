/*	Public domain	*/

typedef struct es_nmos {
	struct es_component _inherit;
	M_Real Vt;			/* Threshold voltage */
	M_Real Va;			/* Early voltage */
	M_Real K;			/* K-value */
	M_Real Ieq, gm, go;		/* Companion model parameters */
	StampVCCSData s_vccs;
	StampConductanceData s_conductance;
	StampCurrentSourceData s_current;
} ES_NMOS;

__BEGIN_DECLS
extern ES_ComponentClass esNMOSClass;
__END_DECLS
