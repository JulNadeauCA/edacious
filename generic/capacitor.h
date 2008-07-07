/*	Public domain	*/

#ifndef _COMPONENT_CAPACITOR_H_
#define _COMPONENT_CAPACITOR_H_

#include "begin_code.h"

typedef struct es_capacitor {
	struct es_component _inherit;
	int vIdx;			/* Index into e */

	M_Real C;
	M_Real V0;

	/* Parameters of the thevenin model */
	M_Real r;
	M_Real v;

	M_Real *s[STAMP_THEVENIN_SIZE];
} ES_Capacitor;

__BEGIN_DECLS
extern ES_ComponentClass esCapacitorClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_CAPACITOR_H_ */
