/*	Public domain	*/

#ifndef _COMPONENT_PNP_H_
#define _COMPONENT_PNP_H_

#include "begin_code.h"

typedef struct es_pnp {
	struct es_component com;
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

	M_Real Ibf_eq_prev;	
	M_Real Ibr_eq_prev;
	M_Real Icc_eq_prev;
	M_Real gPiF_prev;
	M_Real gmF_prev;
	M_Real gPiR_prev;
	M_Real gmR_prev;
	M_Real goPrev;

	M_Real VebPrev;
	M_Real VcbPrev;

	M_Real *sc_be[STAMP_CONDUCTANCE_SIZE];
	M_Real *sc_bc[STAMP_CONDUCTANCE_SIZE];
	M_Real *sc_ec[STAMP_CONDUCTANCE_SIZE];

	M_Real *sv_ebec[STAMP_VCCS_SIZE];
	M_Real *sv_cbce[STAMP_VCCS_SIZE];
	
	M_Real *si_be[STAMP_CURRENT_SOURCE_SIZE];
	M_Real *si_bc[STAMP_CURRENT_SOURCE_SIZE];
	M_Real *si_ce[STAMP_CURRENT_SOURCE_SIZE];

} ES_PNP;

__BEGIN_DECLS
extern ES_ComponentClass esPNPClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_PNP_H_ */
