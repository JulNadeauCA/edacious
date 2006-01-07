/*	$Csoft: inverter.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_INVERTER_H_
#define _COMPONENT_INVERTER_H_

#include <component/component.h>
#include <component/digital.h>

#include "begin_code.h"

typedef ES_Digital ES_Inverter;

__BEGIN_DECLS
void	 ES_InverterInit(void *, const char *);
int	 ES_InverterLoad(void *, AG_Netbuf *);
int	 ES_InverterSave(void *, AG_Netbuf *);
int	 ES_InverterExport(void *, enum circuit_format, FILE *);
void	*ES_InverterEdit(void *);
void	 ES_InverterDraw(void *, VG *);
void	 ES_InverterStep(void *, Uint);
void	 ES_InverterUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_INVERTER_H_ */
