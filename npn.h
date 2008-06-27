/*	Public domain	*/

#ifndef _COMPONENT_NPN_H_
#define _COMPONENT_NPN_H_

#include "component.h"

#include "begin_code.h"

typedef struct es_npn {
	struct es_component com;
	M_Real Vt;		/* Thermal voltage */
	M_Real Va;		/* Early voltage */
	M_Real Ies;
	M_Real alpha;		

	M_Real IBeq;
	M_Real IEeq;
	M_Real gPi;
	M_Real gm;
	M_Real go;

	M_Real IBeqPrev;	
	M_Real IEeqPrev;
	M_Real gPiPrev;
	M_Real gmPrev;
	M_Real goPrev;

	M_Real VbePrev;
	M_Real VcePrev;
} ES_NPN;

__BEGIN_DECLS
extern ES_ComponentClass esNPNClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_NPN_H_ */
