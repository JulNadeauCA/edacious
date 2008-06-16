/*	Public domain	*/

#ifndef _COMPONENT_LOGIC_PROBE_H_
#define _COMPONENT_LOGIC_PROBE_H_

#include "component.h"

#include "begin_code.h"

typedef struct es_logic_probe {
	struct es_component com;
	M_Real Vhigh;		/* Minimum HIGH voltage */
	int state;		/* Last state */
} ES_LogicProbe;

__BEGIN_DECLS
extern ES_ComponentClass esLogicProbeClass;

void ES_LogicProbeUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_LOGIC_PROBE_H_ */
