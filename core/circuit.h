/*	Public domain	*/

#define ESCIRCUIT_DESCR_MAX	512
#define ESCIRCUIT_AUTHORS_MAX	128
#define ESCIRCUIT_KEYWORDS_MAX	128

#define ESCIRCUIT_MAX_BRANCHES	32
#define ESCIRCUIT_MAX_NODES	(0xffff-1)
#define ESCIRCUIT_SYM_MAX	24
#define ESCIRCUIT_SYM_DESCR_MAX	128

struct es_port;
struct es_pair;
struct es_sim;
struct es_sim_ops;

/* Connection to one component. */
typedef struct es_branch {
	struct es_port *port;
	TAILQ_ENTRY(es_branch) branches;
} ES_Branch;

/* Connection between two or more components. */
typedef struct es_node {
	Uint flags;
#define CKTNODE_EXAM		0x01	/* Branches are being examined
					   (used to avoid redundancies) */
#define CKTNODE_REFERENCE	0x02	/* Reference node (eg. ground) */

	TAILQ_HEAD(,es_branch) branches;
	Uint		      nBranches;
} ES_Node;

/* Closed loop of port pairs with respect to some component in the circuit. */
typedef struct es_loop {
	Uint name;
	void *origin;			/* Origin component (ie. vsource) */
	struct es_pair **pairs;		/* Port pairs in loop */
	Uint npairs;
	TAILQ_ENTRY(es_loop) loops;
} ES_Loop;

/* Entry in Circuit symbol table. */
typedef struct es_sym {
	char name[ESCIRCUIT_SYM_MAX];
	char descr[ESCIRCUIT_SYM_DESCR_MAX];
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

struct es_scope;
struct ag_console;

/* The basic Circuit model object. */
typedef struct es_circuit {
	struct ag_object obj;
	char descr[ESCIRCUIT_DESCR_MAX];	/* Description */
	char authors[ESCIRCUIT_AUTHORS_MAX];	/* Authors */
	char keywords[ESCIRCUIT_KEYWORDS_MAX];	/* Keywords */
	VG *vg;					/* Schematics */
	
	struct es_sim *sim;			/* Current simulation mode */
	Uint flags;
#define ES_CIRCUIT_SHOW_NODES		0x01
#define ES_CIRCUIT_SHOW_NODENAMES	0x02
#define ES_CIRCUIT_SHOW_NODESYMS	0x04
	int simlock;			/* Simulation is locked */

	ES_Node **nodes;		/* Nodes (element 0 is ground) */
	ES_Loop **loops;		/* Closed loops */
	struct es_component **vSrcs;	/* Independent voltage sources */
	TAILQ_HEAD(,es_sym) syms;	/* Symbols */

	Uint l;				/* Loops */
	Uint m;				/* Independent voltage sources */
	Uint n;				/* Nodes (except ground) */
	
	struct ag_console *console;	/* Log console */
	struct ag_object **extObjs;	/* External simulation objects */
	Uint              nExtObjs;

	TAILQ_HEAD(,es_component) components;	/* Connected components */
	TAILQ_HEAD(,es_layout) layouts;		/* Associated PCB layouts */
} ES_Circuit;

#define ESCIRCUIT(p) ((ES_Circuit *)(p))

/* Iterate over attached Components in the Circuit. */
#define ESCIRCUIT_FOREACH_COMPONENT(com, ckt) \
	for((com) = AG_TAILQ_FIRST(&ckt->components); \
	    (com) != AG_TAILQ_END(&ckt->components); \
	    (com) = AG_TAILQ_NEXT(ESCOMPONENT(com), components))

/* Iterate over all selected, non-floating Components in the Circuit. */
#define ESCIRCUIT_FOREACH_COMPONENT_SELECTED(com, ckt)		\
	ESCIRCUIT_FOREACH_COMPONENT((com),ckt)			\
		if (!((com)->flags & ES_COMPONENT_SELECTED)) {	\
			continue;				\
		} else

/* Iterate over all Branches of a Node. */
#define ESNODE_FOREACH_BRANCH(br, node) \
	AG_TAILQ_FOREACH(br, &(node)->branches, branches)

#if defined(_ES_INTERNAL) || defined(_USE_EDACIOUS_CORE)
# define CIRCUIT(p) ESCIRCUIT(p)
# define CIRCUIT_FOREACH_COMPONENT(com,ckt) \
	 ESCIRCUIT_FOREACH_COMPONENT((com),(ckt))
# define CIRCUIT_FOREACH_COMPONENT_SELECTED(com,ckt) \
	 ESCIRCUIT_FOREACH_COMPONENT_SELECTED((com),(ckt))
# define NODE_FOREACH_BRANCH(br,node) \
         ESNODE_FOREACH_BRANCH((br),(node))
#endif /* _ES_INTERNAL or _USE_EDACIOUS_CORE */

__BEGIN_DECLS
extern AG_ObjectClass esCircuitClass;
extern VG_ToolOps esSelectTool;
extern VG_ToolOps esComponentInsertTool;
extern VG_ToolOps esWireTool;
extern VG_ToolOps *esCircuitTools[];
extern VG_ToolOps *esSchemTools[];
extern VG_ToolOps *esVgTools[];
extern VG_ToolOps *esLayoutTools[];

ES_Circuit *ES_CircuitNew(void *, const char *);
void        ES_CircuitLog(void *, const char *, ...);
void       *ES_CircuitEdit(void *);
AG_Window  *ES_CircuitOpenObject(void *);
void        ES_CircuitCloseObject(void *);

int         ES_AddNode(ES_Circuit *);
int         ES_AddVoltageSource(ES_Circuit *, void *);
void        ES_DelVoltageSource(ES_Circuit *, int);
void        ES_ClearVoltageSources(ES_Circuit *);
void        ES_DelNode(ES_Circuit *, int);
int         ES_MergeNodes(ES_Circuit *, int, int);
ES_Branch  *ES_AddBranch(ES_Circuit *, int, struct es_port *);
void        ES_DelBranch(ES_Circuit *, int, ES_Branch *);

ES_Node	   *ES_GetNode(ES_Circuit *, int);
void        ES_CopyNodeSymbol(ES_Circuit *, int, char *, size_t);
ES_Node	   *ES_GetNodeBySymbol(ES_Circuit *, const char *);
ES_Branch  *ES_LookupBranch(ES_Circuit *, int, struct es_port *);

M_Real      ES_NodeVoltage(ES_Circuit *, int);
M_Real      ES_NodeVoltagePrevStep(ES_Circuit *, int, int);

M_Real      ES_BranchCurrent(ES_Circuit *, int);
M_Real      ES_BranchCurrentPrevStep(ES_Circuit *, int, int);

void        ES_ResumeSimulation(ES_Circuit *);
void        ES_SuspendSimulation(ES_Circuit *);
struct es_sim *ES_SetSimulationMode(ES_Circuit *, const struct es_sim_ops *);
void        ES_AddSimulationObj(ES_Circuit *, void *);
void        ES_CircuitModified(ES_Circuit *);
void        ES_DestroySimulation(ES_Circuit *);

ES_Sym     *ES_AddSymbol(ES_Circuit *, const char *);
ES_Sym     *ES_AddNodeSymbol(ES_Circuit *, const char *, int);
ES_Sym     *ES_AddVsourceSymbol(ES_Circuit *, const char *, int);
ES_Sym     *ES_AddIsourceSymbol(ES_Circuit *, const char *, int);
void        ES_DelSymbol(ES_Circuit *, ES_Sym *);
ES_Sym     *ES_LookupSymbol(ES_Circuit *, const char *);

/* Lock the circuit and suspend any real-time simulation in progress. */
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

/* Clear component edition areas in the GUI. */
static __inline__ void
ES_ClearEditAreas(VG_View *vv)
{
	Uint i;

	AG_ObjectLock(vv);
	for (i = 0; i < vv->nEditAreas; i++) {
		AG_ObjectFreeChildren(vv->editAreas[i]);
		AG_WindowUpdate(AG_ParentWindow(vv->editAreas[i]));
		AG_WidgetHiddenRecursive(vv->editAreas[i]);
	}
	AG_ObjectUnlock(vv);
}
__END_DECLS
