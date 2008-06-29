/*	Public domain	*/

#ifndef _COMPONENT_CAPACITOR_H_
#define _COMPONENT_CAPACITOR_H_

#include "component.h"

#include "begin_code.h"

typedef struct es_capacitor {
	struct es_component _inherit;
	int vIdx;			/* Index into e */

	M_Real C;
	M_Real V0;

	M_Real g;
	M_Real Ieq;

	M_Real gPrev;
	M_Real IeqPrev;
} ES_Capacitor;

__BEGIN_DECLS
extern ES_ComponentClass esCapacitorClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_CAPACITOR_H_ */
