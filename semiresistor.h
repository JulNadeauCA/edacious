/*	Public domain	*/

#ifndef _COMPONENT_SEMIRESISTOR_H_
#define _COMPONENT_SEMIRESISTOR_H_

#include "component.h"

#include "begin_code.h"

typedef struct es_semiresistor {
	struct es_component com;
	M_Real l, w;		/* Semiconductor length and width (m) */
	M_Real rsh;		/* Resistance (ohms/sq) */
	M_Real defw;		/* Default width */
	M_Real narrow;		/* Narrowing due to side etching */
	M_Real Tc1, Tc2;	/* Resistance/temperature coefficients */
} ES_SemiResistor;

__BEGIN_DECLS
extern ES_ComponentClass esSemiResistorClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_SEMIRESISTOR_H_ */
