/*	Public domain	*/

#ifndef _COMPONENT_VSOURCE_H_
#define _COMPONENT_VSOURCE_H_

#include <eda.h>

#include "begin_code.h"

typedef struct es_vsource {
	struct es_component _inherit;
	int vIdx;			/* Index into e */
	
	M_Real v;			/* Effective voltage */

	TAILQ_HEAD(,es_loop) loops;	/* Forward loops */
	Uint nloops;
	ES_Port	**lstack;		/* For loop computation */
	Uint nlstack;
} ES_Vsource;

#ifdef _ES_INTERNAL
#define VSOURCE(com) ((struct es_vsource *)(com))
#endif

__BEGIN_DECLS
extern ES_ComponentClass esVsourceClass;

void	 ES_VsourceFindLoops(ES_Vsource *);
void	 ES_VsourceFreeLoops(ES_Vsource *);
double	 ES_VsourceVoltage(void *, int, int);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_VSOURCE_H_ */
