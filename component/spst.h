/*	$Csoft: spst.h,v 1.2 2004/08/27 03:16:10 vedge Exp $	*/
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
int		 spst_load(void *, struct netbuf *);
int		 spst_save(void *, struct netbuf *);
int		 spst_export(void *, enum circuit_format, FILE *);
double		 spst_resistance(void *, struct pin *, struct pin *);
struct window	*spst_edit(void *);
void		 spst_draw(void *, struct vg *);
void		 spst_update(void *, struct vg *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_SPST_H_ */
