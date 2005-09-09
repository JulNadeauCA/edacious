/*	$Csoft: ground.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_GROUND_H_
#define _COMPONENT_GROUND_H_

#include <component/component.h>

#include "begin_code.h"

struct ground {
	struct component com;
};

__BEGIN_DECLS
void		 ground_init(void *, const char *);
int		 ground_load(void *, struct netbuf *);
int		 ground_save(void *, struct netbuf *);
int		 ground_export(void *, enum circuit_format, FILE *);
double		 ground_resistance(void *, struct pin *, struct pin *);
struct window	*ground_edit(void *);
int		 ground_configure(void *);
void		 ground_draw(void *, struct vg *);
void		 ground_update(void *, struct vg *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_GROUND_H_ */
