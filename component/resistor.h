/*	$Csoft: resistor.h,v 1.5 2004/08/27 03:16:10 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_RESISTOR_H_
#define _COMPONENT_RESISTOR_H_

#include <component/component.h>

#include "begin_code.h"

struct resistor {
	struct component com;
	double resistance;	/* Resistance at Tnom */
	double power_rating;	/* Power rating in watts */
	int tolerance;		/* Tolerance in % */
	float Tc1, Tc2;		/* Resistance/temperature coefficients */
};

__BEGIN_DECLS
void		 resistor_init(void *, const char *);
int		 resistor_load(void *, struct netbuf *);
int		 resistor_save(void *, struct netbuf *);
int		 resistor_export(void *, enum circuit_format, FILE *);
double		 resistor_resistance(void *, struct pin *, struct pin *);
struct window	*resistor_edit(void *);
int		 resistor_configure(void *);
void		 resistor_draw(void *, struct vg *);
void		 resistor_update(void *, struct vg *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_RESISTOR_H_ */
