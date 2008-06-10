/*	Public domain	*/

#ifndef _COMPONENT_INVERTER_H_
#define _COMPONENT_INVERTER_H_

#include "component.h"
#include "digital.h"

#include "begin_code.h"

typedef ES_Digital ES_Inverter;

__BEGIN_DECLS
extern ES_ComponentClass esInverterClass;

void ES_InverterUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_INVERTER_H_ */
