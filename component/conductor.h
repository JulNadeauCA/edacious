/*	$Csoft: conductor.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_CONDUCTOR_H_
#define _COMPONENT_CONDUCTOR_H_

#include <component/component.h>

#include "begin_code.h"

struct conductor {
	struct component com;
};

__BEGIN_DECLS
void		 conductor_init(void *, const char *);
double		 conductor_resistance(void *, struct pin *, struct pin *);
void		 conductor_draw(void *, VG *);
int		 conductor_export(void *, enum circuit_format, FILE *);
void		 conductor_tool_reinit(void);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_CONDUCTOR_H_ */
