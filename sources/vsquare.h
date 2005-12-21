/*	$Csoft: vsource.h,v 1.3 2005/09/15 02:04:49 vedge Exp $	*/
/*	Public domain	*/

#ifndef _SOURCES_VSQUARE_H_
#define _SOURCES_VSQUARE_H_

#include <component/component.h>
#include <component/vsource.h>

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
void	 ES_VSquareInit(void *, const char *);
void	 ES_VSquareReinit(void *);
void	 ES_VSquareDestroy(void *);
int	 ES_VSquareLoad(void *, AG_Netbuf *);
int	 ES_VSquareSave(void *, AG_Netbuf *);
int	 ES_VSquareExport(void *, enum circuit_format, FILE *);
void	*ES_VSquareEdit(void *);
void	 ES_VSquareDraw(void *, VG *);
__END_DECLS

#include "close_code.h"
#endif /* _SOURCES_VSQUARE_H_ */
