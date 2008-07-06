/*	Public domain	*/

#ifndef _COMPONENT_OR_H_
#define _COMPONENT_OR_H_

#include <macro/digital.h>

#include "begin_code.h"

typedef ES_Digital ES_Or;

__BEGIN_DECLS
extern ES_ComponentClass esOrClass;

void ES_OrUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_OR_H_ */
