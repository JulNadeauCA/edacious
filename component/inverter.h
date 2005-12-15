/*	$Csoft: inverter.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_INVERTER_H_
#define _COMPONENT_INVERTER_H_

#include <component/component.h>

#include "begin_code.h"

typedef struct es_inverter {
	struct es_component com;
	float Tphl, Tplh;		/* Propagation delays for A -> A-bar */
	float Tthl, Ttlh;		/* Transition delay for A-bar */
	float Thold;			/* Minimum hold time for A */
	float Tehl, Telh;		/* Elapsed hold times */
	float Cin;			/* Average input capacitance (pF) */
	float Cpd;			/* Power dissipation capacitance (pF) */
	float Vih, Vil;			/* Input logic threshold */
	float Voh, Vol;			/* Output logic threshold */
} ES_Inverter;

__BEGIN_DECLS
void	 ES_InverterInit(void *, const char *);
int	 ES_InverterLoad(void *, AG_Netbuf *);
int	 ES_InverterSave(void *, AG_Netbuf *);
int	 ES_InverterExport(void *, enum circuit_format, FILE *);
void	*ES_InverterEdit(void *);
void	 ES_InverterTick(void *);
void	 ES_InverterDraw(void *, VG *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_INVERTER_H_ */
