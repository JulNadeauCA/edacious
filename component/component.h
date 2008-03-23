/*	Public domain	*/

#ifndef _COMPONENT_COMPONENT_H_
#define _COMPONENT_COMPONENT_H_

#include <circuit/circuit.h>

#include "begin_code.h"

#define COMPONENT_MAX_PORTS	128
#define COMPONENT_PORT_NAME_MAX	16

enum es_component_fn {
	ES_LOAD_DC_G,		/* Load into conductance matrix */
	ES_LOAD_SP_S,		/* Load into S-parameter matrix */
	ES_LOAD_SP_N,		/* Load into SP noise correlation matrix */
	ES_LOAD_AC_N,		/* Load into AC noise correlation matrix */
	ES_LOAD_AC_Y,		/* Load into AC admittance matrix */
	ES_LOAD_TR,		/* Transient analysis */
	ES_LAST_FN
};

struct es_branch;
struct es_loop;
struct es_circuit;
struct ag_window;
struct ag_menu_item;

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

typedef struct es_spec_condition {
	char name[16];		/* Name of required specification */
	SC_Real min, max;	/* Range of values */
	SLIST_ENTRY(es_spec_condition) conds;
} ES_SpecCondition;

/* Model specification. */
typedef struct es_spec {
	char name[16];				/* Symbol */
	char descr[64];				/* Description */
	SC_Real min, typ, max;			/* Specification values */
	SLIST_HEAD(,es_spec_condition) conds;	/* Test conditions */
} ES_Spec;

/* Generic component description and operation vector. */
typedef struct es_component_class {
	AG_ObjectClass obj;
	const char *name;			/* Name (eg. "Resistor") */
	const char *pfx;			/* Prefix (eg. "R") */

	void	 (*draw)(void *, VG *);
	void	 (*instance_menu)(void *, struct ag_menu_item *);
	void	 (*class_menu)(struct es_circuit *, struct ag_menu_item *);
	int	 (*export_model)(void *, enum circuit_format, FILE *);
	int	 (*connect)(void *, ES_Port *, ES_Port *);
} ES_ComponentClass;

typedef struct es_component {
	struct ag_object obj;
	struct es_circuit *ckt;			/* Back pointer to circuit */
	VG_Block *block;			/* Schematic block */
	Uint flags;
#define COMPONENT_FLOATING	0x01		/* Not yet connected */
	int selected;				/* Selected for edition? */
	int highlighted;			/* Selected for selection? */
	pthread_mutex_t lock;
	AG_Timeout redraw_to;			/* Timer for vg updates */
	float Tspec;				/* Instance temp (k) */
	ES_Port ports[COMPONENT_MAX_PORTS];	/* Interface elements */
	int    nports;
	ES_Pair *pairs;				/* Port pairs */
	int     npairs;
	ES_Spec	*specs;				/* Model specifications */
	int	nspecs;

	int (*loadDC_G)(void *, SC_Matrix *G);
	int (*loadDC_BCD)(void *, SC_Matrix *B, SC_Matrix *C, SC_Matrix *D);
	int (*loadDC_RHS)(void *, SC_Vector *i, SC_Vector *e);
	int (*loadAC)(void *, SC_Matrix *Y);
	int (*loadSP)(void *, SC_Matrix *S, SC_Matrix *N);
	void (*intStep)(void *, Uint);
	void (*intUpdate)(void *);

	TAILQ_ENTRY(es_component) components;
} ES_Component;

#define COMOPS(p) ((ES_ComponentClass *)(AGOBJECT(p)->cls))
#define COM(p) ((ES_Component *)(p))
#define PORT(p,n) (&COM(p)->ports[n])
#define PAIR(p,n) (&COM(p)->pairs[n])
#define COM_Z0 50.0				/* Normalizing impedance */
#define COM_T0 290.0				/* Reference temperature */
#define PVOLTAGE(p,n) ES_NodeVoltage(COM(p)->ckt,PNODE((p),(n)))

#ifdef DEBUG
#define PNODE(p,n) ES_PortNode(COM(p),(n))
#else
#define PNODE(p,n) (COM(p)->ports[n].node)
#endif

__BEGIN_DECLS
extern AG_ObjectClass esComponentClass;

void	 ES_ComponentFreePorts(ES_Component *);
void	 ES_ComponentSetPorts(void *, const ES_Port *);

void	 ES_ComponentOpenMenu(ES_Component *, VG_View *);
void	 ES_ComponentCloseMenu(VG_View *);
void	 ES_ComponentInsert(AG_Event *);
ES_Port	*ES_ComponentPortOverlap(struct es_circuit *, ES_Component *, float,
		                 float);
int	 ES_ComponentHighlightPorts(struct es_circuit *, ES_Component *);
void	 ES_ComponentConnect(struct es_circuit *, ES_Component *, VG_Vtx *);
void	 ES_ComponentSelect(ES_Component *);
void	 ES_ComponentUnselect(ES_Component *);
void	 ES_ComponentUnselectAll(struct es_circuit *);
void	 ES_ComponentLog(void *, const char *, ...);

Uint	 ES_PortNode(ES_Component *, int);
int	 ES_PortIsGrounded(ES_Port *);
int	 ES_PairIsInLoop(ES_Pair *, struct es_loop *, int *);
ES_Port	*ES_FindPort(void *, const char *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_COMPONENT_H_ */
