/*	Public domain	*/

#ifndef _COMPONENT_ISOURCE_H_
#define _COMPONENT_ISOURCE_H_

#include "component.h"

#include "begin_code.h"

typedef struct es_isource {
	struct es_component com;

	M_Real I;
} ES_Isource;

__BEGIN_DECLS
extern ES_ComponentClass esIsourceClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_ISOURCE_H_ */
