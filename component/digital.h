/*	$Csoft: resistor.h,v 1.2 2005/09/15 02:04:49 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_DIGITAL_H_
#define _COMPONENT_DIGITAL_H_

#include <component/component.h>

#include "begin_code.h"

typedef enum es_digital_class {
	ES_DIGITAL_GATE,
	ES_DIGITAL_BUFFERS,
	ES_DIGITAL_FLIPFLOPS,
	ES_DIGITAL_MSI,
	ES_DIGITAL_LSI
} ES_DigitalClass;

typedef enum es_digital_family {
	ES_HE4000B

} ES_DigitalFamily;

typedef struct es_digital {
	struct es_component com;
	SC_Vector *G;		/* Current conductive state */
	SC_Range Vdd;		/* Supply voltage */
	SC_Range Vi;		/* Voltage on any input */
	SC_Range Tamb;		/* Operating ambient temperature */
	SC_Range Idd;		/* Quiescent current */
	SC_Range Vol, Voh;	/* Output voltage LOW/HIGH (buffered) */
	SC_Range Vil, Vih;	/* Input voltage LOW/HIGH (buffered) */
	SC_Range VolUB, VohUB;	/* Output voltage LOW/HIGH (unbuf) */
	SC_Range VilUB, VihUB;	/* Input voltage LOW/HIGH (unbuf) */
	SC_Range Iol;		/* Output (sink current); LOW */
	SC_Range Ioh;		/* Output (source current); HIGH */
	SC_Range Iin;		/* Input leakage current (+-) */
	SC_Range Iozh;		/* 3-state output leakage current; HIGH */
	SC_Range Iozl;		/* 3-state output leakage current; LOW */
	SC_QTimeRange Tthl;	/* Output transition time; HIGH->LOW */
	SC_QTimeRange Ttlh;	/* Output transition time; LOW->HIGH */
} ES_Digital;

#define DIG(p) ((ES_Digital *)(p))
#define digG(p,j) (DIG(p)->G->mat[j][1])
#define digInHI(p,j) (U(p,j) >= DIG(p)->Vih)
#define digInLO(p,j) (U(p,j) <= DIG(p)->Vil)
#define digOutHI(p,j) (U(p,j) >= DIG(p)->Voh)
#define digOutLO(p,j) (U(p,j) <= DIG(p)->Vol)

__BEGIN_DECLS
void	 ES_DigitalInit(void *, const char *, const char *, const void *,
	                const ES_Port *);
int	 ES_DigitalLoad(void *, AG_Netbuf *);
int	 ES_DigitalSave(void *, AG_Netbuf *);
int	 ES_DigitalExport(void *, enum circuit_format, FILE *);
void	 ES_DigitalEdit(void *, void *);
void	 ES_DigitalDraw(void *, VG *);

void	 ES_DigitalSwitch(void *, int, int);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_DIGITAL_H_ */
