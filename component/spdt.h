/*	$Csoft: spdt.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_SPDT_H_
#define _COMPONENT_SPDT_H_

#include <component/component.h>

#include "begin_code.h"

typedef struct es_spdt {
	struct es_component com;
	double on_resistance;
	double off_resistance;
	int state;
} ES_Spdt;

__BEGIN_DECLS
void		 ES_SpdtInit(void *, const char *);
int		 ES_SpdtLoad(void *, AG_Netbuf *);
int		 ES_SpdtSave(void *, AG_Netbuf *);
int		 ES_SpdtExport(void *, enum circuit_format, FILE *);
double		 ES_SpdtResistance(void *, ES_Port *, ES_Port *);
AG_Window	*ES_SpdtEdit(void *);
void		 ES_SpdtDraw(void *, VG *);
void		 ES_SpdtUpdate(void *, VG *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_SPDT_H_ */
