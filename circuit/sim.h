/*	$Csoft: sim.h,v 1.1 2005/09/08 09:46:01 vedge Exp $	*/
/*	Public domain	*/

#ifndef _CIRCUIT_SIM_H_
#define _CIRCUIT_SIM_H_
#include "begin_code.h"

struct sim {
	struct circuit *ckt;
	const struct sim_ops *ops;
	AG_Window *win;
	int running;
};

struct sim_ops {
	char *name;
	int icon;
	size_t size;
	void (*init)(void *);
	void (*destroy)(void *);
	void (*cktmod)(void *, struct circuit *);
	AG_Window *(*edit)(void *, struct circuit *);
};

#define SIM(p) ((struct sim *)p)

__BEGIN_DECLS
void sim_init(void *, const struct sim_ops *);
void sim_destroy(void *);
void sim_edit(AG_Event *);
__END_DECLS

#include "close_code.h"
#endif	/* _CIRCUIT_SIM_H_ */
