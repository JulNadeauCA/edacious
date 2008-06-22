/*	Public domain	*/

#ifndef _SOURCES_VSWEEP_H_
#define _SOURCES_VSWEEP_H_

#include "vsource.h"

#include "begin_code.h"

typedef struct es_vsweep {
	struct es_vsource _inherit;
	M_Real v1;			/* Start voltage */
	M_Real v2;			/* End voltage */
	M_Time t;			/* Duration */
	int count;			/* Repeat count (0 = loop) */

	M_Real dv;
	M_Time tElapsed;
	int cycle;
} ES_VSweep;

#define ES_VSWEEP(com) ((struct es_vsweep *)(com))

__BEGIN_DECLS
extern ES_ComponentClass esVSweepClass;
__END_DECLS

#include "close_code.h"
#endif /* _SOURCES_VSWEEP_H_ */
