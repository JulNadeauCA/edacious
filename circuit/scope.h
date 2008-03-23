/*	Public domain	*/

#ifndef _AGAR_EDA_CIRCUIT_SCOPE_H_
#define _AGAR_EDA_CIRCUIT_SCOPE_H_

#include "begin_code.h"

typedef struct ag_scope {
	struct ag_object obj;
	ES_Circuit *ckt;
} ES_Scope;

__BEGIN_DECLS
extern AG_ObjectClass esScopeClass;
ES_Scope *ES_ScopeNew(void *, const char *);
__END_DECLS

#include "close_code.h"
#endif	/* _AGAR_EDA_CIRCUIT_SCOPE_H_ */
