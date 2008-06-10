/*	Public domain	*/

#ifndef _COMPONENT_DIODE_H_
#define _COMPONENT_DIODE_H_

#include "component.h"

#include "begin_code.h"

typedef struct es_diode {
	struct es_component com;
	SC_Real Is;		/* Resistance at Tnom */
	SC_Real Vt;		/* Thermal voltage */
} ES_Diode;

__BEGIN_DECLS
extern ES_ComponentClass esDiodeClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_DIODE_H_ */
