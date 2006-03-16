/*	$Csoft: sim.h,v 1.1 2005/09/08 09:46:01 vedge Exp $	*/
/*	Public domain	*/

#ifndef _CIRCUIT_SIM_H_
#define _CIRCUIT_SIM_H_
#include "begin_code.h"

struct es_circuit;
struct ag_console;

typedef struct es_sim_ops {
	char *name;
	int icon;
	size_t size;
	void (*init)(void *);
	void (*destroy)(void *);
	void (*start)(void *);
	void (*stop)(void *);
	void (*cktmod)(void *, struct es_circuit *);
	SC_Real (*node_voltage)(void *, int);
	SC_Real (*branch_current)(void *, int);
	AG_Window *(*edit)(void *, struct es_circuit *);
} ES_SimOps;

typedef struct es_sim {
	struct es_circuit *ckt;		/* Circuit being analyzed */
	const ES_SimOps *ops;		/* Generic operations */
	AG_Window *win;			/* Settings window */
	int running;			/* Continous simulation in process */
} ES_Sim;

#define SIM(p) ((ES_Sim *)p)

__BEGIN_DECLS
void ES_SimInit(void *, const ES_SimOps *);
void ES_SimDestroy(void *);
void ES_SimEdit(AG_Event *);
void ES_SimLog(void *, const char *, ...);
__END_DECLS

#include "close_code.h"
#endif	/* _CIRCUIT_SIM_H_ */
