/*	Public domain	*/

#ifndef _COMPONENT_AND_H_
#define _COMPONENT_AND_H_

#include <component/component.h>
#include <component/digital.h>

#include "begin_code.h"

typedef ES_Digital ES_And;

__BEGIN_DECLS
void	 ES_AndInit(void *, const char *);
int	 ES_AndLoad(void *, AG_DataSource *);
int	 ES_AndSave(void *, AG_DataSource *);
void	*ES_AndEdit(void *);
void	 ES_AndDraw(void *, VG *);
void	 ES_AndUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_AND_H_ */
