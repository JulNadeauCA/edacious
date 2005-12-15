/*	$Csoft: vsource.h,v 1.3 2005/09/15 02:04:49 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_VSOURCE_H_
#define _COMPONENT_VSOURCE_H_

#include <component/component.h>

#include "begin_code.h"

typedef struct es_vsource {
	struct es_component com;
	double voltage;
	TAILQ_HEAD(,es_loop) loops;	/* Forward loops */
	Uint nloops;
	ES_Port	**lstack;		/* Temporary loop stack */
	Uint nlstack;
} ES_Vsource;

#define VSOURCE(com) ((struct es_vsource *)(com))

__BEGIN_DECLS
void	 ES_VsourceInit(void *, const char *);
void	 ES_VsourceReinit(void *);
void	 ES_VsourceDestroy(void *);
void	 ES_VsourceFindLoops(ES_Vsource *);
void	 ES_VsourceFreeLoops(ES_Vsource *);
int	 ES_VsourceLoad(void *, AG_Netbuf *);
int	 ES_VsourceSave(void *, AG_Netbuf *);
int	 ES_VsourceExport(void *, enum circuit_format, FILE *);
double	 ES_VsourceVoltage(void *, int, int);
void	*ES_VsourceEdit(void *);
void	 ES_VsourceDraw(void *, VG *);

__inline__ int ES_VsourceName(ES_Vsource *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_VSOURCE_H_ */
