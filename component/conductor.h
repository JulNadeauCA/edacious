/*	$Csoft: conductor.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_CONDUCTOR_H_
#define _COMPONENT_CONDUCTOR_H_

#include <component/component.h>

#include "begin_code.h"

typedef struct es_conductor {
	struct es_component com;
} ES_Conductor;

__BEGIN_DECLS
void	 ES_ConductorInit(void *, const char *);
double	 ES_ConductorResistance(void *, ES_Port *, ES_Port *);
void	 ES_ConductorDraw(void *, VG *);
int	 ES_ConductorExport(void *, enum circuit_format, FILE *);
void	 ES_ConductorToolReinit(void);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_CONDUCTOR_H_ */
