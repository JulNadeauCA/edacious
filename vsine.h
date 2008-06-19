/*	Public domain	*/

#ifndef _SOURCES_VSINE_H_
#define _SOURCES_VSINE_H_

#include "vsource.h"

#include "begin_code.h"

typedef struct es_vsine {
	struct es_vsource vs;		/* Derived from independent vsource */
	M_Real vPeak;			/* Peak voltage */
	M_Real f;			/* Frequency (Hz) */
	M_Real phase;

	M_Real vPrev;
} ES_VSine;

#define ES_VSINE(com) ((struct es_vsine *)(com))

__BEGIN_DECLS
extern ES_ComponentClass esVSineClass;

void	 ES_VSineInit(void *, const char *);
void	 ES_VSineReinit(void *);
void	 ES_VSineDestroy(void *);
int	 ES_VSineLoad(void *, AG_DataSource *);
int	 ES_VSineSave(void *, AG_DataSource *);
int	 ES_VSineExport(void *, enum circuit_format, FILE *);
void	 ES_VSineUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _SOURCES_VSINE_H_ */
