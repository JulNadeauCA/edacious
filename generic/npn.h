/*	Public domain	*/

typedef struct es_npn {
	struct es_component _inherit;

	M_Real Vt;		/* Thermal voltage */
	M_Real Va;		/* Early voltage */
	M_Real Ifs,Irs;
	M_Real betaF,betaR;		

	M_Real Ibf_eq;
	M_Real Ibr_eq;
	M_Real Icc_eq;
	M_Real gPiF;
	M_Real gmF;
	M_Real gPiR;
	M_Real gmR;
	M_Real go;

	M_Real VbePrevIter;
	M_Real VbcPrevIter;

	StampConductanceData sc_be;
	StampConductanceData sc_bc;
	StampConductanceData sc_ec;

	StampVCCSData sv_bece;
	StampVCCSData sv_bcec;
	
	StampCurrentSourceData si_eb;
	StampCurrentSourceData si_cb;
	StampCurrentSourceData si_ec;
} ES_NPN;

__BEGIN_DECLS
extern ES_ComponentClass esNPNClass;
__END_DECLS
