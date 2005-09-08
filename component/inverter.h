/*	$Csoft: inverter.h,v 1.4 2004/08/27 03:16:10 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_INVERTER_H_
#define _COMPONENT_INVERTER_H_

#include <component/component.h>

#include "begin_code.h"

struct inverter {
	struct component com;
	float Tphl, Tplh;		/* Propagation delays for A -> A-bar */
	float Tthl, Ttlh;		/* Transition delay for A-bar */
	float Thold;			/* Minimum hold time for A */
	float Tehl, Telh;		/* Elapsed hold times */
	float Cin;			/* Average input capacitance (pF) */
	float Cpd;			/* Power dissipation capacitance (pF) */
	float Vih, Vil;			/* Input logic threshold */
	float Voh, Vol;			/* Output logic threshold */
};

__BEGIN_DECLS
void		 inverter_init(void *, const char *);
int		 inverter_load(void *, struct netbuf *);
int		 inverter_save(void *, struct netbuf *);
int		 inverter_export(void *, enum circuit_format, FILE *);
struct window	*inverter_edit(void *);
void		 inverter_tick(void *);
void		 inverter_draw(void *, struct vg *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_INVERTER_H_ */
