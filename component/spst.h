/*	$Csoft: spst.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_SPST_H_
#define _COMPONENT_SPST_H_

#include <component/component.h>

#include "begin_code.h"

struct spst {
	struct component com;
	double on_resistance;
	double off_resistance;
	int state;
};

__BEGIN_DECLS
void		 spst_init(void *, const char *);
int		 spst_load(void *, AG_Netbuf *);
int		 spst_save(void *, AG_Netbuf *);
int		 spst_export(void *, enum circuit_format, FILE *);
double		 spst_resistance(void *, struct pin *, struct pin *);
AG_Window	*spst_edit(void *);
void		 spst_draw(void *, VG *);
void		 spst_update(void *, VG *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_SPST_H_ */
