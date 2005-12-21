/*	$Csoft: inverter.h,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_INVERTER_H_
#define _COMPONENT_INVERTER_H_

#include <component/component.h>
#include <component/digital.h>

#include "begin_code.h"

typedef struct es_inverter {
	struct es_digital com;
	SC_QTime Tphl, Tplh;	/* Propagation delays for A -> A-bar */
	SC_QTime Tthl, Ttlh;	/* Transition delay for A-bar */
	SC_QTime Thold;		/* Minimum hold time for A */
	SC_QTime Tehl, Telh;	/* Elapsed hold times */
	SC_Real Cin;		/* Average input capacitance (pF) */
	SC_Real Cpd;		/* Power dissipation capacitance (pF) */
	SC_Real Vih, Vil;	/* Input logic threshold */
	SC_Real Voh, Vol;	/* Output logic threshold */
} ES_Inverter;

__BEGIN_DECLS
void	 ES_InverterInit(void *, const char *);
int	 ES_InverterLoad(void *, AG_Netbuf *);
int	 ES_InverterSave(void *, AG_Netbuf *);
int	 ES_InverterExport(void *, enum circuit_format, FILE *);
void	*ES_InverterEdit(void *);
void	 ES_InverterDraw(void *, VG *);
void	 ES_InverterStep(void *, Uint);
void	 ES_InverterUpdate(void *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_INVERTER_H_ */
