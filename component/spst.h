/*	Public domain	*/

#ifndef _COMPONENT_SPST_H_
#define _COMPONENT_SPST_H_

#include <component/component.h>

#include "begin_code.h"

typedef struct es_spst {
	ES_Component com;
	double on_resistance;
	double off_resistance;
	int state;
} ES_Spst;

__BEGIN_DECLS
void	 ES_SpstInit(void *, const char *);
int	 ES_SpstLoad(void *, AG_DataSource *);
int	 ES_SpstSave(void *, AG_DataSource *);
int	 ES_SpstExport(void *, enum circuit_format, FILE *);
double	 ES_SpstResistance(void *, ES_Port *, ES_Port *);
void	*ES_SpstEdit(void *);
void	 ES_SpstDraw(void *, VG *);
void	 ES_SpstUpdate(void *, VG *);
void	 ES_SpstClassMenu(ES_Circuit *, AG_MenuItem *);
void	 ES_SpstInstanceMenu(void *, AG_MenuItem *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_SPST_H_ */
