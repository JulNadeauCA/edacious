/*	Public domain	*/

#ifndef _COMPONENT_RESISTOR_H_
#define _COMPONENT_RESISTOR_H_

#include "component.h"

#include "begin_code.h"

typedef struct es_resistor {
	struct es_component com;
	SC_Real resistance;	/* Resistance at Tnom */
	SC_Real power_rating;	/* Power rating in watts */
	int tolerance;		/* Tolerance in % */
	float Tc1, Tc2;		/* Resistance/temperature coefficients */
} ES_Resistor;

__BEGIN_DECLS
extern ES_ComponentClass esResistorClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_RESISTOR_H_ */
