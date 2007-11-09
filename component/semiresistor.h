/*	Public domain	*/

#ifndef _COMPONENT_SEMIRESISTOR_H_
#define _COMPONENT_SEMIRESISTOR_H_

#include <component/component.h>

#include "begin_code.h"

typedef struct es_semiresistor {
	struct es_component com;
	double l, w;		/* Semiconductor length and width */
	double rsh;		/* Resistance (ohms/sq) */
	double defw;		/* Default width */
	double narrow;		/* Narrowing due to side etching */
	float Tc1, Tc2;		/* Resistance/temperature coefficients */
} ES_SemiResistor;

__BEGIN_DECLS
extern const ES_ComponentOps esSemiResistorOps;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_SEMIRESISTOR_H_ */
