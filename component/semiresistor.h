/*	$Csoft: semiresistor.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
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
void		 ES_SemiResistorInit(void *, const char *);
int		 ES_SemiResistorLoad(void *, AG_Netbuf *);
int		 ES_SemiResistorSave(void *, AG_Netbuf *);
int		 ES_SemiResistorExport(void *, enum circuit_format, FILE *);
double		 ES_SemiResistorResistance(void *, ES_Port *, ES_Port *);
AG_Window	*ES_SemiResistorEdit(void *);
int		 ES_SemiResistorConfigure(void *);
void		 ES_SemiResistorDraw(void *, VG *);
void		 ES_SemiResistorUpdate(void *, VG *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_SEMIRESISTOR_H_ */
