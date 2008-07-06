/*	Public domain	*/

#ifndef _COMPONENT_PMOS_H_
#define _COMPONENT_PMOS_H_

#include "begin_code.h"

typedef struct es_pmos {
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
} ES_PMOS;

__BEGIN_DECLS
extern ES_ComponentClass esPMOSClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_PMOS_H_ */
