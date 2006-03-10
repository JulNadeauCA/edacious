/*	$Csoft: inverter.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_OR_H_
#define _COMPONENT_OR_H_

#include <component/component.h>
#include <component/digital.h>

#include "begin_code.h"

typedef ES_Digital ES_Or;

__BEGIN_DECLS
void	 ES_OrInit(void *, const char *);
int	 ES_OrLoad(void *, AG_Netbuf *);
int	 ES_OrSave(void *, AG_Netbuf *);
void	*ES_OrEdit(void *);
void	 ES_OrDraw(void *, VG *);
void	 ES_OrUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_OR_H_ */
