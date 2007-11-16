/*	Public domain	*/

#ifndef _COMPONENT_GROUND_H_
#define _COMPONENT_GROUND_H_

#include <component/component.h>

#include "begin_code.h"

typedef struct es_ground {
	struct es_component com;
} ES_Ground;

__BEGIN_DECLS
extern const ES_ComponentClass esGroundClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_GROUND_H_ */
