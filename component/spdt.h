/*	$Csoft: spdt.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
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
int		 spdt_load(void *, AG_Netbuf *);
int		 spdt_save(void *, AG_Netbuf *);
int		 spdt_export(void *, enum circuit_format, FILE *);
double		 spdt_resistance(void *, struct pin *, struct pin *);
AG_Window	*spdt_edit(void *);
void		 spdt_draw(void *, VG *);
void		 spdt_update(void *, VG *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_SPDT_H_ */
