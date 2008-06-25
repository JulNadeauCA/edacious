/*	Public domain	*/

#ifndef _SOURCES_VARB_H_
#define _SOURCES_VARB_H_

#include "vsource.h"

#include "begin_code.h"

#define EXP_MAX_SIZE 20
typedef struct es_varb {
	struct es_vsource vs;		/* Derived from independent vsource */
	char exp[EXP_MAX_SIZE];		/* Expression to interpret */
} ES_VArb;

#define ES_VARB(com) ((struct es_varb *)(com))

__BEGIN_DECLS
extern ES_ComponentClass esVArbClass;

void	 ES_VArbInit(void *, const char *);
void	 ES_VArbReinit(void *);
void	 ES_VArbDestroy(void *);
int	 ES_VArbLoad(void *, AG_DataSource *);
int	 ES_VArbSave(void *, AG_DataSource *);
int	 ES_VArbExport(void *, enum circuit_format, FILE *);
void	 ES_VArbUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _SOURCES_VARB_H_ */
