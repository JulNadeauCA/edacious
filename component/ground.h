/*	$Csoft: ground.h,v 1.2 2005/09/15 02:04:49 vedge Exp $	*/
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
int		 ground_load(void *, AG_Netbuf *);
int		 ground_save(void *, AG_Netbuf *);
int		 ground_export(void *, enum circuit_format, FILE *);
double		 ground_resistance(void *, struct pin *, struct pin *);
AG_Window	*ground_edit(void *);
int		 ground_configure(void *);
void		 ground_draw(void *, VG *);
void		 ground_update(void *, VG *);
int		 ground_connect(void *, struct pin *, struct pin *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_GROUND_H_ */
