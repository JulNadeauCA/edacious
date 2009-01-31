/*	Public domain	*/

typedef struct es_pnp {
	struct es_component _inherit;

	M_Real Vt;			/* Thermal voltage */
	M_Real Va;			/* Early voltage */
	M_Real Ifs, Irs;
	M_Real betaF, betaR;		

	M_Real Ibf_eq;
	M_Real Ibr_eq;
	M_Real Icc_eq;
	M_Real gPiF;
	M_Real gmF;
	M_Real gPiR;
	M_Real gmR;
	M_Real go;

	M_Real VebPrevIter;
	M_Real VcbPrevIter;

	StampConductanceData sc_be;
	StampConductanceData sc_bc;
	StampConductanceData sc_ec;

	StampVCCSData sv_ebec;
	StampVCCSData sv_cbce;
	
	StampCurrentSourceData si_be;
	StampCurrentSourceData si_bc;
	StampCurrentSourceData si_ce;
} ES_PNP;

__BEGIN_DECLS
extern ES_ComponentClass esPNPClass;
__END_DECLS
