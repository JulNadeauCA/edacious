/*	Public domain	*/

#ifndef _AGAR_EDA_CIRCUIT_SCOPE_H_
#define _AGAR_EDA_CIRCUIT_SCOPE_H_

#include "begin_code.h"

typedef struct ag_scope {
	struct ag_object obj;
	ES_Circuit *ckt;
} ES_Scope;

#define AG_SCOPE(ob) ((ES_Scope *)(ob))

__BEGIN_DECLS
ES_Scope *ES_ScopeNew(void *, const char *);
void	  ES_ScopeInit(void *, const char *);
void	  ES_ScopeReinit(void *);
void	  ES_ScopeDestroy(void *);
void	 *ES_ScopeEdit(void *);
int	  ES_ScopeLoad(void *, AG_DataSource *);
int	  ES_ScopeSave(void *, AG_DataSource *);
__END_DECLS

#include "close_code.h"
#endif	/* _AGAR_EDA_CIRCUIT_SCOPE_H_ */
