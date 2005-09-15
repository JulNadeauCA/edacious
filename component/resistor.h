/*	$Csoft: resistor.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
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
int		 resistor_connect(void *, struct pin *, struct pin *);
void		 resistor_draw(void *, struct vg *);
void		 resistor_update(void *, struct vg *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_RESISTOR_H_ */
