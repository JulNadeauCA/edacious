/*	$Csoft: inverter.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
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
int		 inverter_load(void *, AG_Netbuf *);
int		 inverter_save(void *, AG_Netbuf *);
int		 inverter_export(void *, enum circuit_format, FILE *);
AG_Window	*inverter_edit(void *);
void		 inverter_tick(void *);
void		 inverter_draw(void *, VG *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_INVERTER_H_ */
