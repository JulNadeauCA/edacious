/*	Public domain	*/

#define ESCIRCUIT_DESCR_MAX	512
#define ESCIRCUIT_AUTHORS_MAX	128
#define ESCIRCUIT_KEYWORDS_MAX	128

#define ESCIRCUIT_MAX_BRANCHES	32
#define ESCIRCUIT_MAX_NODES	(0xffff-1)
#define ESCIRCUIT_SYM_MAX	24
#define ESCIRCUIT_SYM_DESCR_MAX	128

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
	Uint		      nBranches;
} ES_Node;

/* Closed loop of port pairs with respect to some component in the circuit. */
typedef struct es_loop {
	Uint name;
	void *origin;			/* Origin component (ie. vsource) */
	ES_Pair **pairs;		/* Port pairs in loop */
	Uint	 npairs;
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
	
	ES_Sim *sim;				/* Current simulation mode */
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
} ES_Circuit;

#define ESCIRCUIT(p) ((ES_Circuit *)(p))

/* Iterate over all Components of the Circuit, floating or not. */
#define ESCIRCUIT_FOREACH_COMPONENT_ALL(com, ckt) \
	AGOBJECT_FOREACH_CHILD((com),(ckt),es_component)

/* Iterate over all non-floating Components in the Circuit. */
#define ESCIRCUIT_FOREACH_COMPONENT(com, ckt)			\
	ESCIRCUIT_FOREACH_COMPONENT_ALL((com),ckt)		\
		if ((com)->flags & ES_COMPONENT_FLOATING) {	\
			continue;				\
		} else

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
# define CIRCUIT_FOREACH_COMPONENT_ALL(com,ckt) \
	 ESCIRCUIT_FOREACH_COMPONENT_ALL((com),(ckt))
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
extern VG_ToolOps esInsertTool;
extern VG_ToolOps esWireTool;

ES_Circuit *ES_CircuitNew(void *, const char *);
void	    ES_CircuitLog(void *, const char *, ...);

int         ES_AddNode(ES_Circuit *);
int         ES_AddVoltageSource(ES_Circuit *, void *);
void        ES_DelVoltageSource(ES_Circuit *, int);
void        ES_ClearVoltageSources(ES_Circuit *);
void        ES_DelNode(ES_Circuit *, int);
int         ES_MergeNodes(ES_Circuit *, int, int);
ES_Branch  *ES_AddBranch(ES_Circuit *, int, ES_Port *);
void        ES_DelBranch(ES_Circuit *, int, ES_Branch *);

ES_Node	   *ES_GetNode(ES_Circuit *, int);
void        ES_CopyNodeSymbol(ES_Circuit *, int, char *, size_t);
ES_Node	   *ES_GetNodeBySymbol(ES_Circuit *, const char *);
ES_Branch  *ES_LookupBranch(ES_Circuit *, int, ES_Port *);

M_Real      ES_NodeVoltage(ES_Circuit *, int);
M_Real      ES_NodeVoltagePrevStep(ES_Circuit *, int, int);

M_Real      ES_BranchCurrent(ES_Circuit *, int);
M_Real      ES_BranchCurrentPrevStep(ES_Circuit *, int, int);

void        ES_ResumeSimulation(ES_Circuit *);
void        ES_SuspendSimulation(ES_Circuit *);
ES_Sim     *ES_SetSimulationMode(ES_Circuit *, const ES_SimOps *);
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

/* Select/unselect components. */
static __inline__ void
ES_SelectComponent(ES_Component *com, VG_View *vv)
{
	com->flags |= ES_COMPONENT_SELECTED;

	if (vv->nEditAreas > 0) {
		AG_ObjectFreeChildren(vv->editAreas[0]);
		AG_ObjectAttach(vv->editAreas[0], esComponentClass.edit(com));
		AG_WindowUpdate(AG_ParentWindow(vv->editAreas[0]));
		AG_WidgetShownRecursive(vv->editAreas[0]);
	}
}
static __inline__ void
ES_UnselectComponent(ES_Component *com, VG_View *vv)
{
	com->flags &= ~(ES_COMPONENT_SELECTED);
}
static __inline__ void
ES_SelectAllComponents(ES_Circuit *ckt, VG_View *vv)
{
	ES_Component *com;

	ESCIRCUIT_FOREACH_COMPONENT(com, ckt)
		com->flags |= ES_COMPONENT_SELECTED;
}
static __inline__ void
ES_UnselectAllComponents(ES_Circuit *ckt, VG_View *vv)
{
	ES_Component *com;

	ESCIRCUIT_FOREACH_COMPONENT(com, ckt) {
		com->flags &= ~(ES_COMPONENT_SELECTED);
	}
	ES_ClearEditAreas(vv);
}
/* Highlight component for selection. */
static __inline__ void
ES_HighlightComponent(ES_Component *hCom)
{
	ES_Circuit *ckt = ESCOMPONENT_CIRCUIT(hCom);
	ES_Component *com;

	ESCIRCUIT_FOREACH_COMPONENT(com, ckt) {
		com->flags &= ~(ES_COMPONENT_HIGHLIGHTED);
	}
	hCom->flags |= ES_COMPONENT_HIGHLIGHTED;
}

/* Select/unselect component ports. */
static __inline__ void
ES_SelectPort(ES_Port *port)
{
	port->flags |= ES_PORT_SELECTED;
}
static __inline__ void
ES_UnselectPort(ES_Port *port)
{
	port->flags &= ~(ES_PORT_SELECTED);
}
static __inline__ void
ES_UnselectAllPorts(struct es_circuit *ckt)
{
	ES_Component *com;
	ES_Port *port;
	int i;

	ESCIRCUIT_FOREACH_COMPONENT(com, ckt) {
		ESCOMPONENT_FOREACH_PORT(port, i, com)
			ES_UnselectPort(port);
	}
}
__END_DECLS
