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

typedef enum es_digital_state {
	ES_LOW,			/* Logical LOW */
	ES_HIGH,		/* Logical HIGH */
	ES_HI_Z,		/* High impedance output */
	ES_INVALID		/* Invalid logic input */
} ES_LogicState;

typedef struct es_digital {
	struct es_component com;
	int VccPort;		/* DC supply port */
	int GndPort;		/* Ground port */
	SC_Vector *G;		/* Current conductive state */
	SC_Range Vcc;		/* DC supply voltage (operating) */
	SC_Range Tamb;		/* Operating ambient temperature */
	SC_Range Idd;		/* Quiescent current */
	SC_Range Vol, Voh;	/* Output voltage LOW/HIGH (buffered) */
	SC_Range Vil, Vih;	/* Input voltage LOW/HIGH (buffered) */
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
extern const ES_ComponentOps esDigitalOps;

void	 ES_DigitalDraw(void *, VG *);
void	 ES_DigitalSetVccPort(void *, int);
void	 ES_DigitalSetGndPort(void *, int);
int	 ES_LogicOutput(void *, const char *, ES_LogicState);
int	 ES_LogicInput(void *, const char *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_DIGITAL_H_ */
