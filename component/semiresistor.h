/*	$Csoft: semiresistor.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_SEMIRESISTOR_H_
#define _COMPONENT_SEMIRESISTOR_H_

#include <component/component.h>

#include "begin_code.h"

struct semiresistor {
	struct component com;
	double l, w;		/* Semiconductor length and width */
	double rsh;		/* Resistance (ohms/sq) */
	double defw;		/* Default width */
	double narrow;		/* Narrowing due to side etching */
	float Tc1, Tc2;		/* Resistance/temperature coefficients */
};

__BEGIN_DECLS
void		 semiresistor_init(void *, const char *);
int		 semiresistor_load(void *, AG_Netbuf *);
int		 semiresistor_save(void *, AG_Netbuf *);
int		 semiresistor_export(void *, enum circuit_format, FILE *);
double		 semiresistor_resistance(void *, struct pin *, struct pin *);
AG_Window	*semiresistor_edit(void *);
int		 semiresistor_configure(void *);
void		 semiresistor_draw(void *, VG *);
void		 semiresistor_update(void *, VG *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_SEMIRESISTOR_H_ */
