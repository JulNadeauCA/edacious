/*	Public domain	*/

#ifndef _CIRCUIT_CIRCUIT_H_
#define _CIRCUIT_CIRCUIT_H_

#include <agar/core.h>
#include <agar/vg.h>
#include <agar/sc.h>

#include <agar/config/_mk_have_unsigned_typedefs.h>
#ifndef _MK_HAVE_UNSIGNED_TYPEDEFS
#define Uchar unsigned char
#define Uint unsigned int
#define Ulong unsigned long
#endif

#include <component/component.h>
#include <circuit/sim.h>

#include "begin_code.h"

#define CIRCUIT_NAME_MAX	256
#define CIRCUIT_DESCR_MAX	256
#define CIRCUIT_MAX_BRANCHES	32
#define CIRCUIT_MAX_NODES	(0xffff-1)
#define CKTNODE_SYM_MAX		24
#define CIRCUIT_SYM_MAX		24
#define CIRCUIT_SYM_DESCR_MAX	128

/* Connection to one component. */
typedef struct es_branch {
	ES_Port *port;
	TAILQ_ENTRY(es_branch) branches;
} ES_Branch;

/* Connection between two or more components. */
typedef struct es_node {
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

typedef struct es_sym {
	char name[CIRCUIT_SYM_MAX];
	char descr[CIRCUIT_SYM_DESCR_MAX];
	enum es_sym_type {
		ES_SYM_NODE,
		ES_SYM_VSOURCE,
		ES_SYM_ISOURCE,
	} type;
	union {
		int node;
		int vsource;
		int isource;
	} p;
	TAILQ_ENTRY(es_sym) syms;
} ES_Sym;

struct es_vsource;
struct ag_console;

typedef struct es_circuit {
	struct ag_object obj;
	char descr[CIRCUIT_DESCR_MAX];	/* Short description */
	VG *vg;				/* Schematics drawing */
	VG_Block *vgblk;		/* Circuit display block */
	struct ag_console *console;	/* Log console */
	ES_Sim *sim;			/* Current simulation mode */
	Uint flags;
#define CIRCUIT_SHOW_NODES	0x01
#define CIRCUIT_SHOW_NODENAMES	0x02
#define CIRCUIT_SHOW_NODESYMS	0x04
	int simlock;			/* Simulation is locked */

	ES_Node **nodes;		/* Nodes (element 0 is ground) */
	ES_Loop **loops;		/* Closed loops */
	struct es_vsource **vsrcs;	/* Independent vsources */
	TAILQ_HEAD(,es_wire) wires;	/* Schematic lines */
	TAILQ_HEAD(,es_sym) syms;	/* Symbols */

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
extern AG_ObjectClass esCircuitClass;

void	 ES_CircuitFreeComponents(ES_Circuit *);
void	 ES_CircuitLog(void *, const char *, ...);

int	 ES_CircuitAddNode(ES_Circuit *, Uint);
int	 ES_CircuitMergeNodes(ES_Circuit *, int, int);
void	 ES_CircuitDelNode(ES_Circuit *, int);
int	 ES_CircuitDelNodeByPtr(ES_Circuit *, ES_Node *);

void		 ES_CircuitCopyNode(ES_Circuit *, ES_Node *, ES_Node *);
void		 ES_CircuitFreeNode(ES_Node *);
ES_Branch	*ES_CircuitAddBranch(ES_Circuit *, int, ES_Port *);
void		 ES_CircuitDelBranch(ES_Circuit *, int, ES_Branch *);
ES_Wire		*ES_CircuitAddWire(ES_Circuit *);
void		 ES_CircuitDelWire(ES_Circuit *, ES_Wire *);
void	 	 ES_CircuitDrawPort(ES_Circuit *, ES_Port *, float, float);

ES_Node		*ES_CircuitGetNode(ES_Circuit *, int);
void		 ES_CircuitNodeSymbol(ES_Circuit *, int, char *,
			                      size_t);
ES_Node		*ES_CircuitFindNode(ES_Circuit *, const char *);
ES_Branch	*ES_CircuitGetBranch(ES_Circuit *, int, ES_Port *);
int		 ES_CircuitNodeNameByPtr(ES_Circuit *, ES_Node *);

int		 ES_NodeVsource(ES_Circuit *, int, int, int *);
SC_Real		 ES_NodeVoltage(ES_Circuit *, int);
SC_Real		 ES_BranchCurrent(ES_Circuit *, int);

void	 	 ES_ResumeSimulation(ES_Circuit *);
void	 	 ES_SuspendSimulation(ES_Circuit *);
ES_Sim		*ES_SetSimulationMode(ES_Circuit *, const ES_SimOps *);
void		 ES_CircuitModified(ES_Circuit *);
void		 ES_DestroySimulation(ES_Circuit *);

ES_Sym		*ES_CircuitAddSym(ES_Circuit *);
ES_Sym		*ES_CircuitAddNodeSym(ES_Circuit *, const char *, int);
ES_Sym		*ES_CircuitAddVsourceSym(ES_Circuit *, const char *, int);
ES_Sym		*ES_CircuitAddIsourceSym(ES_Circuit *, const char *, int);
void		 ES_CircuitDelSym(ES_Circuit *, ES_Sym *);
ES_Sym 		*ES_CircuitFindSym(ES_Circuit *, const char *);

/* Lock the circuit and suspend any continuous simulation in progress. */
static __inline__ void
ES_LockCircuit(ES_Circuit *ckt)
{
	AG_ObjectLock(ckt);
	if (ckt->sim != NULL && ckt->sim->running) {
		if (ckt->simlock++ == 0)
			ES_SuspendSimulation(ckt);
	}
}

/* Resume any previously suspended simulation and unlock the circuit. */
static __inline__ void
ES_UnlockCircuit(ES_Circuit *ckt)
{
	if (ckt->sim != NULL && !ckt->sim->running) {
		if (--ckt->simlock == 0)
			ES_ResumeSimulation(ckt);
	}
	AG_ObjectUnlock(ckt);
}
__END_DECLS

#include "close_code.h"
#endif	/* _CIRCUIT_CIRCUIT_H_ */
