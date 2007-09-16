/*	Public domain	*/

#ifndef _COMPONENT_RESISTOR_H_
#define _COMPONENT_RESISTOR_H_

#include <component/component.h>

#include "begin_code.h"

typedef struct es_resistor {
	struct es_component com;
	SC_Real resistance;	/* Resistance at Tnom */
	SC_Real power_rating;	/* Power rating in watts */
	int tolerance;		/* Tolerance in % */
	float Tc1, Tc2;		/* Resistance/temperature coefficients */
} ES_Resistor;

__BEGIN_DECLS
void	 ES_ResistorInit(void *, const char *);
int	 ES_ResistorLoad(void *, AG_Netbuf *);
int	 ES_ResistorSave(void *, AG_Netbuf *);
int	 ES_ResistorExport(void *, enum circuit_format, FILE *);
SC_Real	 ES_ResistorResistance(void *, ES_Port *, ES_Port *);
void	*ES_ResistorEdit(void *);
int	 ES_ResistorConnect(void *, ES_Port *, ES_Port *);
void	 ES_ResistorDraw(void *, VG *);
void	 ES_ResistorUpdate(void *, VG *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_RESISTOR_H_ */
