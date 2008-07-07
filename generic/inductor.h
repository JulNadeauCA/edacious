/*	Public domain	*/

#ifndef _COMPONENT_INDUCTOR_H_
#define _COMPONENT_INDUCTOR_H_

#include "begin_code.h"

typedef struct es_inductor {
	struct es_component com;

	M_Real L;

	M_Real g;
	M_Real Ieq;

	M_Real gPrev;
	M_Real IeqPrev;

	M_Real *s_conductance[STAMP_CONDUCTANCE_SIZE];
	M_Real *s_current_source[STAMP_CONDUCTANCE_SIZE];
} ES_Inductor;

__BEGIN_DECLS
extern ES_ComponentClass esInductorClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_INDUCTOR_H_ */
