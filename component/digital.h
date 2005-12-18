/*	$Csoft: resistor.h,v 1.2 2005/09/15 02:04:49 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_DIGITAL_H_
#define _COMPONENT_DIGITAL_H_

#include <component/component.h>

#include "begin_code.h"

typedef struct es_digital {
	struct es_component com;
	SC_Vector *G;			/* Conductances between ports */
} ES_Digital;

#define DIG(p) ((ES_Digital *)(p))

__BEGIN_DECLS
void	 ES_DigitalInit(void *, const char *, const char *, const void *,
	                const ES_Port *);
int	 ES_DigitalLoad(void *, AG_Netbuf *);
int	 ES_DigitalSave(void *, AG_Netbuf *);
int	 ES_DigitalExport(void *, enum circuit_format, FILE *);
void	*ES_DigitalEdit(void *);
void	 ES_DigitalDraw(void *, VG *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_DIGITAL_H_ */
