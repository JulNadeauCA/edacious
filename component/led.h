/*	Public domain	*/

#ifndef _COMPONENT_LED_H_
#define _COMPONENT_LED_H_

#include <component/component.h>

#include "begin_code.h"

typedef struct es_led {
	struct es_component com;
	SC_Real Vforw;		/* Forward voltage */
	SC_Real Vrev;		/* Reverse voltage */
	SC_Real I;		/* Luminous intensity (mcd) */
	int state;
} ES_Led;

__BEGIN_DECLS
void	 ES_LedInit(void *, const char *);
int	 ES_LedLoad(void *, AG_Netbuf *);
int	 ES_LedSave(void *, AG_Netbuf *);
void	*ES_LedEdit(void *);
int	 ES_LedConnect(void *, ES_Port *, ES_Port *);
void	 ES_LedDraw(void *, VG *);
void	 ES_LedUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_LED_H_ */
