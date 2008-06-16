/*	Public domain	*/

#ifndef _COMPONENT_LED_H_
#define _COMPONENT_LED_H_

#include "component.h"

#include "begin_code.h"

typedef struct es_led {
	struct es_component com;
	M_Real Vforw;		/* Forward voltage */
	M_Real Vrev;		/* Reverse voltage */
	M_Real I;		/* Luminous intensity (mcd) */
	int state;
} ES_Led;

__BEGIN_DECLS
extern ES_ComponentClass esLedClass;

int	 ES_LedConnect(void *, ES_Port *, ES_Port *);
void	 ES_LedDraw(void *, VG *);
void	 ES_LedUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_LED_H_ */
