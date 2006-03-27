/*	$Csoft: circuit.h,v 1.5 2005/09/15 02:04:48 vedge Exp $	*/
/*	Public domain	*/

#ifndef _CIRCUIT_CIRCUIT_H_
#define _CIRCUIT_CIRCUIT_H_

#include <agar/core.h>
#include <agar/vg.h>
#include <agar/sc.h>

#include <component/component.h>
#include <circuit/sim.h>

#include "begin_code.h"

#define CIRCUIT_NAME_MAX	256
#define CIRCUIT_DESCR_MAX	256
#define CIRCUIT_MAX_BRANCHES	32
#define CIRCUIT_MAX_NODES	(0xffff-1)
#define CKTNODE_SYM_MAX		24

/* Connection to one component. */
typedef struct es_branch {
	ES_Port *port;
	TAILQ_ENTRY(es_branch) branches;
} ES_Branch;

/* Connection between two or more components. */
typedef struct es_node {
	char sym[CKTNODE_SYM_MAX];	/* Symbolic name */
	Uint flags;
#define CKTNODE_EXAM		0x01	/* Branches are being examined
					   (used to avoid redundancies) */
#define CKTNODE_REFERENCE	0x02	/* Reference node (eg. ground) */

	TAILQ_HEAD(,es_branch) branches;
	Uint		      nbranches;
} ES_Node;

typedef struct es_wire {
	Uint flags;
#define ES_WIRE_FIXED	0x01		/* Don't allow moving */
	Uint cat;			/* Category */
	struct es_port ports[2];	/* Ports */
	TAILQ_ENTRY(es_wire) wires;
} ES_Wire;

/* Closed loop of port pairs with respect to some component in the circuit. */
typedef struct es_loop {
	Uint name;
	void *origin;			/* Origin component (ie. vsource) */
	ES_Pair **pairs;		/* Port pairs in loop */
	Uint	 npairs;
	TAILQ_ENTRY(es_loop) loops;
} ES_Loop;

struct es_vsource;
struct ag_console;

typedef struct es_circuit {
	AG_Object obj;
	char descr[CIRCUIT_DESCR_MAX];	/* Short description */
	VG *vg;				/* Schematics drawing */
	VG_Block *vgblk;		/* Circuit display block */
	struct ag_console *console;	/* Log console */
	ES_Sim *sim;			/* Current simulation mode */
	Uint flags;
#define CIRCUIT_SHOW_NODES	0x01
#define CIRCUIT_SHOW_NODENAMES	0x02
#define CIRCUIT_SHOW_NODESYMS	0x04
	pthread_mutex_t lock;
	int simlock;			/* Simulation is locked */

	ES_Node **nodes;		/* Nodes (element 0 is ground) */
	ES_Loop **loops;		/* Closed loops */
	struct es_vsource **vsrcs;	/* Independent vsources */
	TAILQ_HEAD(,es_wire) wires;	/* Schematic lines */

	Uint l;			/* Number of loops */
	Uint m;			/* Number of independent vsources */
	Uint n;			/* Number of nodes (except ground) */
} ES_Circuit;

#define ES_CIRCUIT(p) ((ES_Circuit *)(p))
#define U(com,n) ES_NodeVoltage(COM(com)->ckt,(n))
#define ESNODE_FOREACH_BRANCH(var, ckt, node) \
	TAILQ_FOREACH((var), &(ckt)->nodes[node]->branches, branches)
#define ES_NODE(ckt,n) ES_CircuitGetNode((ckt),(n))

__BEGIN_DECLS
void		 ES_CircuitInit(void *, const char *);
void		 ES_CircuitReinit(void *);
int		 ES_CircuitLoad(void *, AG_Netbuf *);
int		 ES_CircuitSave(void *, AG_Netbuf *);
void		 ES_CircuitDestroy(void *);
void		 ES_CircuitFreeComponents(ES_Circuit *);
void		*ES_CircuitEdit(void *);
void		 ES_CircuitLog(void *, const char *, ...);
__inline__ void	 ES_LockCircuit(ES_Circuit *);
__inline__ void	 ES_UnlockCircuit(ES_Circuit *);

int		 ES_CircuitAddNode(ES_Circuit *, Uint);
int		 ES_CircuitMergeNodes(ES_Circuit *, int, int);
void		 ES_CircuitDelNode(ES_Circuit *, int);
int		 ES_CircuitDelNodeByPtr(ES_Circuit *, ES_Node *);

void		 ES_CircuitCopyNode(ES_Circuit *, ES_Node *, ES_Node *);
void		 ES_CircuitFreeNode(ES_Node *);
ES_Branch	*ES_CircuitAddBranch(ES_Circuit *, int, ES_Port *);
void		 ES_CircuitDelBranch(ES_Circuit *, int, ES_Branch *);
ES_Wire		*ES_CircuitAddWire(ES_Circuit *);
void		 ES_CircuitDelWire(ES_Circuit *, ES_Wire *);
__inline__ void	 ES_CircuitDrawPort(ES_Circuit *, ES_Port *, float, float);

__inline__ ES_Node	*ES_CircuitGetNode(ES_Circuit *, int);
__inline__ ES_Node	*ES_CircuitFindNode(ES_Circuit *, const char *);
__inline__ ES_Branch	*ES_CircuitGetBranch(ES_Circuit *, int, ES_Port *);
__inline__ int		 ES_CircuitNodeNameByPtr(ES_Circuit *, ES_Node *);

__inline__ int	   ES_NodeVsource(ES_Circuit *, int, int, int *);
__inline__ SC_Real ES_NodeVoltage(ES_Circuit *, int);
__inline__ SC_Real ES_BranchCurrent(ES_Circuit *, int);

__inline__ void	 ES_ResumeSimulation(ES_Circuit *);
__inline__ void	 ES_SuspendSimulation(ES_Circuit *);
ES_Sim		*ES_SetSimulationMode(ES_Circuit *, const ES_SimOps *);
void		 ES_CircuitModified(ES_Circuit *);
void		 ES_DestroySimulation(ES_Circuit *);

__END_DECLS

#include "close_code.h"
#endif	/* _CIRCUIT_CIRCUIT_H_ */
