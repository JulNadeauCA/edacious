/*	Public domain	*/

#ifndef _COMPONENT_AND_H_
#define _COMPONENT_AND_H_

#include <component/component.h>
#include <component/digital.h>

#include "begin_code.h"

typedef ES_Digital ES_And;

__BEGIN_DECLS
extern ES_ComponentClass esAndClass;

void ES_AndUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_AND_H_ */
