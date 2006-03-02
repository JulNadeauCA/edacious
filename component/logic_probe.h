/*	$Csoft: resistor.h,v 1.2 2005/09/15 02:04:49 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_LOGIC_PROBE_H_
#define _COMPONENT_LOGIC_PROBE_H_

#include <component/component.h>

#include "begin_code.h"

typedef struct es_logic_probe {
	struct es_component com;
	SC_Real Vhigh;		/* Minimum HIGH voltage */
	int state;		/* Last state */
} ES_LogicProbe;

__BEGIN_DECLS
void	 ES_LogicProbeInit(void *, const char *);
int	 ES_LogicProbeLoad(void *, AG_Netbuf *);
int	 ES_LogicProbeSave(void *, AG_Netbuf *);
void	*ES_LogicProbeEdit(void *);
int	 ES_LogicProbeConnect(void *, ES_Port *, ES_Port *);
void	 ES_LogicProbeDraw(void *, VG *);
void	 ES_LogicProbeUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_LOGIC_PROBE_H_ */
