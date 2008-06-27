/*	Public domain	*/

#ifndef _COMPONENT_NPN_H_
#define _COMPONENT_NPN_H_

#include "component.h"

#include "begin_code.h"

typedef struct es_npn {
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

	M_Real VbePrev;
	M_Real VbcPrev;
} ES_NPN;

__BEGIN_DECLS
extern ES_ComponentClass esNPNClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_NPN_H_ */
