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

/* Connection to one component. */
typedef struct es_branch {
	ES_Port *port;
	TAILQ_ENTRY(es_branch) branches;
} ES_Branch;

/* Connection between two or more components. */
typedef struct es_node {
	u_int flags;
#define CKTNODE_EXAM		0x01	/* Branches are being examined
					   (used to avoid redundancies) */
#define CKTNODE_REFERENCE	0x02	/* Reference node (eg. ground) */

	TAILQ_HEAD(,es_branch) branches;
	u_int		      nbranches;
} ES_Node;

/* Closed loop of port pairs with respect to some component in the circuit. */
typedef struct es_loop {
	u_int name;
	void *origin;			/* Origin component (ie. vsource) */
	ES_Pair **pairs;		/* Port pairs in loop */
	u_int	 npairs;
	TAILQ_ENTRY(es_loop) loops;
} ES_Loop;

struct es_vsource;

typedef struct es_circuit {
	AG_Object obj;
	char descr[CIRCUIT_DESCR_MAX];	/* Short description */
	VG *vg;				/* Schematics drawing */
	ES_Sim *sim;			/* Current simulation mode */

	pthread_mutex_t lock;
	int simlock;			/* Simulation is locked */
	int dis_nodes;			/* Display node indications */
	int dis_node_names;		/* Display node names */

	ES_Node **nodes;		/* Nodes (element 0 is ground) */
	ES_Loop **loops;		/* Closed loops */
	struct es_vsource **vsrcs;	/* Independent vsources */

	u_int l;			/* Number of loops */
	u_int m;			/* Number of independent vsources */
	u_int n;			/* Number of nodes (except ground) */
} ES_Circuit;

#define ES_CIRCUIT(p) ((ES_Circuit *)(p))
#define U(com,n) ES_NodeVoltage(COM(com)->ckt,(n))

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

int		 ES_CircuitAddNode(ES_Circuit *, u_int);
void		 ES_CircuitDelNode(ES_Circuit *, u_int);
void		 ES_CircuitCopyNode(ES_Circuit *, ES_Node *, ES_Node *);
void		 ES_CircuitDestroyNode(ES_Node *);
ES_Branch	*ES_CircuitAddBranch(ES_Circuit *, u_int, ES_Port *);
void		 ES_CircuitDelBranch(ES_Circuit *, u_int, ES_Branch *);

__inline__ ES_Branch *ES_CircuitGetBranch(ES_Circuit *, u_int, ES_Port *);

__inline__ int	   ES_NodeGrounded(ES_Circuit *, u_int);
__inline__ int	   ES_NodeVsource(ES_Circuit *, u_int, u_int, int *);
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
