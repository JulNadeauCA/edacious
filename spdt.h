/*	Public domain	*/

#ifndef _COMPONENT_SPDT_H_
#define _COMPONENT_SPDT_H_

#include "component.h"

#include "begin_code.h"

typedef struct es_spdt {
	struct es_component com;
	M_Real rOn;
	M_Real rOff;
	int state;
} ES_Spdt;

__BEGIN_DECLS
extern ES_ComponentClass esSpdtClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_SPDT_H_ */
