/*	$Csoft: component.h,v 1.4 2005/09/15 02:04:49 vedge Exp $	*/
/*	Public domain	*/

#ifndef _COMPONENT_COMPONENT_H_
#define _COMPONENT_COMPONENT_H_

#include <circuit/circuit.h>

#include "begin_code.h"

#define COMPONENT_MAX_PORTS	128
#define COMPONENT_PORT_NAME_MAX	16

struct es_branch;
struct es_loop;
struct es_circuit;
struct ag_window;

/* Exportable circuit model formats. */
enum circuit_format {
	CIRCUIT_SPICE3				/* SPICE3 deck */
};

/* Interface element between the model and the circuit. */
typedef struct es_port {
	int n;					/* Port number */
	char name[COMPONENT_PORT_NAME_MAX];	/* Port name */
	float x, y;				/* Position in drawing */
	struct es_component *com;		/* Back pointer to component */
	int node;				/* Node connection (or -1) */
	struct es_branch *branch;		/* Branch into node */
	int selected;				/* Port selected for edition */
	double u;				/* Potential (fictitious) */
} ES_Port;

/* Ordered pair of ports belonging to the same component. */
typedef struct es_pair {
	struct es_component *com;	/* Back pointer to component */
	ES_Port *p1;			/* + with respect to reference */
	ES_Port *p2;			/* - with respect to reference */
	struct es_loop **loops;		/* Loops this pair is part of */
	int *lpols;			/* Polarities with respect to loops */
	unsigned int nloops;
} ES_Pair;

/* Generic component description and operation vector. */
typedef struct es_component_ops {
	AG_ObjectOps ops;			/* Generic object ops */
	const char *name;			/* Name (eg. "Resistor") */
	const char *pfx;			/* Prefix (eg. "R") */

	void		 (*draw)(void *, VG *);
	struct ag_window *(*edit)(void *);
	int		 (*connect)(void *, ES_Port *, ES_Port *);
	int		 (*export_model)(void *, enum circuit_format, FILE *);
	void		 (*tick)(void *);
	double		 (*resistance)(void *, ES_Port *, ES_Port *);
	double		 (*capacitance)(void *, ES_Port *, ES_Port *);
	double		 (*inductance)(void *, ES_Port *, ES_Port *);
	double		 (*isource)(void *, ES_Port *, ES_Port *);
} ES_ComponentOps;

typedef struct es_component {
	AG_Object obj;
	const ES_ComponentOps *ops;
	struct es_circuit *ckt;			/* Back pointer to circuit */
	VG_Block *block;			/* Schematic block */
	u_int flags;
#define COMPONENT_FLOATING	0x01		/* Not yet connected */
	int selected;				/* Selected for edition? */
	int highlighted;			/* Selected for selection? */
	pthread_mutex_t lock;
	AG_Timeout redraw_to;			/* Timer for vg updates */
	float Tnom;				/* Model ref temp (k) */
	float Tspec;				/* Instance temp (k) */
	ES_Port ports[COMPONENT_MAX_PORTS];	/* Interface elements */
	int    nports;
	ES_Pair *pairs;				/* Port pairs */
	int     npairs;
	TAILQ_ENTRY(es_component) components;
} ES_Component;

#define COM(p) ((struct es_component *)(p))
#define PORT(p,n) (&COM(p)->ports[n])
#define PNODE(p,n) (COM(p)->ports[n].node)
#define U(p,n) (PORT((p),(n))->u)

__BEGIN_DECLS
void	 ES_ComponentInit(void *, const char *, const char *, const void *,
		          const ES_Port *);
void	 ES_ComponentDestroy(void *);
int	 ES_ComponentLoad(void *, AG_Netbuf *);
int	 ES_ComponentSave(void *, AG_Netbuf *);
void	*ES_ComponentEdit(void *);
void	 ES_ComponentInsert(AG_Event *);
ES_Port	*ES_ComponentPortOverlap(struct es_circuit *, ES_Component *, float,
		                 float);
int	 ES_ComponentHighlightPorts(struct es_circuit *, ES_Component *);
void	 ES_ComponentConnect(struct es_circuit *, ES_Component *, VG_Vtx *);
void	 ES_ComponentSelect(ES_Component *);
void	 ES_ComponentUnselect(ES_Component *);
void	 ES_ComponentUnselectAll(struct es_circuit *);

__inline__ int  ES_PortIsGrounded(ES_Port *);
double		ES_PairResistance(ES_Pair *);
double		ES_PairCapacitance(ES_Pair *);
double		ES_PairInductance(ES_Pair *);
__inline__ int	ES_PairIsInLoop(ES_Pair *, struct es_loop *, int *);

#ifdef EDITION
void		ES_ComponentRegMenu(AG_Menu *, AG_MenuItem *,
		                    struct es_circuit *);
#endif
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_COMPONENT_H_ */
