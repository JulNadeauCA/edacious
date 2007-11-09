/*	Public domain	*/

#ifndef _SOURCES_VSQUARE_H_
#define _SOURCES_VSQUARE_H_

#include <sources/vsource.h>

#include "begin_code.h"

typedef struct es_vsquare {
	struct es_vsource vs;		/* Derived from independent vsource */
	SC_Real vH;			/* Voltage of high signal */
	SC_Real vL;			/* Voltage of low signal */
	SC_QTime tH;			/* High pulse duration */
	SC_QTime tL;			/* Low pulse duration */
	SC_QTime t;			/* Current pulse duration */
} ES_VSquare;

#define ES_VSQUARE(com) ((struct es_vsquare *)(com))

__BEGIN_DECLS
void	 ES_VSquareStep(void *, Uint);
void	 ES_VSquareUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _SOURCES_VSQUARE_H_ */
