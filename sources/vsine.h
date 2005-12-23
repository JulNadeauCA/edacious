/*	$Csoft: vsource.h,v 1.3 2005/09/15 02:04:49 vedge Exp $	*/
/*	Public domain	*/

#ifndef _SOURCES_VSINE_H_
#define _SOURCES_VSINE_H_

#include <sources/vsource.h>

#include "begin_code.h"

typedef struct es_vsine {
	struct es_vsource vs;		/* Derived from independent vsource */
	SC_Real vPeak;			/* Peak voltage */
	SC_Real f;			/* Frequency (Hz) */
	SC_Real phase;
} ES_VSine;

#define ES_VSINE(com) ((struct es_vsine *)(com))

__BEGIN_DECLS
void	 ES_VSineInit(void *, const char *);
void	 ES_VSineReinit(void *);
void	 ES_VSineDestroy(void *);
int	 ES_VSineLoad(void *, AG_Netbuf *);
int	 ES_VSineSave(void *, AG_Netbuf *);
int	 ES_VSineExport(void *, enum circuit_format, FILE *);
void	*ES_VSineEdit(void *);
void	 ES_VSineDraw(void *, VG *);
void	 ES_VSineStep(void *, Uint);
void	 ES_VSineUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _SOURCES_VSINE_H_ */
