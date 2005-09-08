/*	$Csoft: spdt.h,v 1.2 2004/08/27 03:16:10 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_SPDT_H_
#define _COMPONENT_SPDT_H_

#include <component/component.h>

#include "begin_code.h"

struct spdt {
	struct component com;
	double on_resistance;
	double off_resistance;
	int state;
};

__BEGIN_DECLS
void		 spdt_init(void *, const char *);
int		 spdt_load(void *, struct netbuf *);
int		 spdt_save(void *, struct netbuf *);
int		 spdt_export(void *, enum circuit_format, FILE *);
double		 spdt_resistance(void *, struct pin *, struct pin *);
struct window	*spdt_edit(void *);
void		 spdt_draw(void *, struct vg *);
void		 spdt_update(void *, struct vg *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_SPDT_H_ */
