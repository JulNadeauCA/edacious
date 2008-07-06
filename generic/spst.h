/*	Public domain	*/

#ifndef _COMPONENT_SPST_H_
#define _COMPONENT_SPST_H_

#include "begin_code.h"

typedef struct es_spst {
	ES_Component com;
	M_Real rOn;
	M_Real rOff;
	int state;
} ES_Spst;

__BEGIN_DECLS
extern ES_ComponentClass esSpstClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_SPST_H_ */
