/*	$Csoft: conductor.h,v 1.4 2004/08/27 03:16:10 vedge Exp $	*/
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
struct window	*conductor_edit(void *);
void		 conductor_draw(void *, struct vg *);
int		 conductor_export(void *, enum circuit_format, FILE *);
void		 conductor_tool_reinit(void);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_CONDUCTOR_H_ */
