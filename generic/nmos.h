/*	Public domain	*/

#ifndef _COMPONENT_NMOS_H_
#define _COMPONENT_NMOS_H_

#include "begin_code.h"

typedef struct es_nmos {
	struct es_component com;
	M_Real Vt;		/* Threshold voltage */
	M_Real Va;		/* Early voltage */
	M_Real K;		/* K-value */

	M_Real Ieq;
	M_Real gm;
	M_Real go;

	M_Real IeqPrev;	
	M_Real gmPrev;
	M_Real goPrev;

	M_Real *s_vccs[STAMP_VCCS_SIZE];
	M_Real *s_conductance[STAMP_CONDUCTANCE_SIZE];
	M_Real *s_current[STAMP_CURRENT_SOURCE_SIZE];
} ES_NMOS;

__BEGIN_DECLS
extern ES_ComponentClass esNMOSClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_NMOS_H_ */
