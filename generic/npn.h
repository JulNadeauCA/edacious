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

	M_Real *sc_be[STAMP_CONDUCTANCE_SIZE];
	M_Real *sc_bc[STAMP_CONDUCTANCE_SIZE];
	M_Real *sc_ec[STAMP_CONDUCTANCE_SIZE];

	M_Real *sv_bece[STAMP_VCCS_SIZE];
	M_Real *sv_bcec[STAMP_VCCS_SIZE];
	
	M_Real *si_eb[STAMP_CURRENT_SOURCE_SIZE];
	M_Real *si_cb[STAMP_CURRENT_SOURCE_SIZE];
	M_Real *si_ec[STAMP_CURRENT_SOURCE_SIZE];
} ES_NPN;

__BEGIN_DECLS
extern ES_ComponentClass esNPNClass;
__END_DECLS
