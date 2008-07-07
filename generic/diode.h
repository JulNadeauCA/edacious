/*	Public domain	*/

#ifndef _COMPONENT_DIODE_H_
#define _COMPONENT_DIODE_H_

#include "begin_code.h"

typedef struct es_diode {
	struct es_component com;
	M_Real Is;		/* Resistance at Tnom */
	M_Real Vt;		/* Thermal voltage */

	M_Real Ieq;
	M_Real g;

	M_Real vPrev;
	M_Real IeqPrev;	
	M_Real gPrev;

	/* Guess at each beginning of a time step for the voltage,
	 * used for N-R algorithm.
	 * May be updated to improve efficiency.*/
	M_Real prevGuess;

	M_Real *s_conductance[STAMP_CONDUCTANCE_SIZE];
	M_Real *s_current_source[STAMP_CONDUCTANCE_SIZE];
} ES_Diode;

__BEGIN_DECLS
extern ES_ComponentClass esDiodeClass;
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_DIODE_H_ */
