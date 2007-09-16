/*	Public domain	*/

#ifndef _COMPONENT_GROUND_H_
#define _COMPONENT_GROUND_H_

#include <component/component.h>

#include "begin_code.h"

typedef struct es_ground {
	struct es_component com;
} ES_Ground;

__BEGIN_DECLS
void	 ES_GroundInit(void *, const char *);
int	 ES_GroundLoad(void *, AG_Netbuf *);
int	 ES_GroundSave(void *, AG_Netbuf *);
int	 ES_GroundExport(void *, enum circuit_format, FILE *);
double	 ES_GroundResistance(void *, ES_Port *, ES_Port *);
void	*ES_GroundEdit(void *);
int	 ES_GroundConfigure(void *);
void	 ES_GroundDraw(void *, VG *);
void	 ES_GroundUpdate(void *, VG *);
int	 ES_GroundConnect(void *, ES_Port *, ES_Port *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_GROUND_H_ */
