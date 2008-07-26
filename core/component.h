/*	Public domain	*/

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
struct es_sim_dc;

/* Exportable circuit model formats. */
enum circuit_format {
	CIRCUIT_SPICE3				/* SPICE3 deck */
};

/* Interface element between the model and the circuit. */
typedef struct es_port {
	int n;					/* Port number */
	char name[COMPONENT_PORT_NAME_MAX];	/* Port name */
	struct es_component *com;		/* Component object */
	int node;				/* Node connection (or -1) */
	struct es_branch *branch;		/* Branch into node */
	Uint flags;
#define ES_PORT_SELECTED	0x01		/* Port is selected */
	ES_SchemPort *sp;			/* Schematic port */
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
	M_Real min, max;	/* Range of values */
	SLIST_ENTRY(es_spec_condition) conds;
} ES_SpecCondition;

/* Model specification. */
typedef struct es_spec {
	char name[16];				/* Symbol */
	char descr[64];				/* Description */
	M_Real min, typ, max;			/* Specification values */
	SLIST_HEAD(,es_spec_condition) conds;	/* Test conditions */
} ES_Spec;

/* Generic component description and operation vector. */
typedef struct es_component_class {
	AG_ObjectClass obj;
	const char *name;	/* Name (e.g., "Resistor") */
	const char *pfx;	/* Prefix (e.g., "R") */
	const char *schemFile;	/* Schematic filename (or NULL if generated) */
	const char *categories;	/* Classes */
	struct ag_static_icon *icon;

	void	 (*draw)(void *, VG *);
	void	 (*instance_menu)(void *, struct ag_menu_item *);
	void	 (*class_menu)(struct es_circuit *, struct ag_menu_item *);
	int	 (*export_model)(void *, enum circuit_format, FILE *);
	int	 (*connect)(void *, ES_Port *, ES_Port *);
} ES_ComponentClass;

typedef struct es_component {
	struct ag_object obj;
	struct es_circuit *ckt;			/* Back pointer to circuit */
	Uint flags;
#define ES_COMPONENT_FLOATING	 0x01		/* Not yet connected */
#define ES_COMPONENT_SELECTED	 0x02		/* Selected for edition */
#define ES_COMPONENT_HIGHLIGHTED 0x04		/* Highlighted for selection */
#define ES_COMPONENT_SUPPRESSED	 0x08		/* Become zero voltage source */
#define ES_COMPONENT_SAVED_FLAGS (ES_COMPONENT_SUPPRESSED)

	M_Real Tspec;				/* Instance temp (k) */
	ES_Port ports[COMPONENT_MAX_PORTS];	/* Ports (indices 1..nports) */
	Uint   nports;
	ES_Pair *pairs;				/* Port pairs */
	Uint    npairs;
	ES_Spec	*specs;				/* Model specifications */
	Uint	nspecs;

	int (*dcSimPrep)(void *, struct es_sim_dc *);
	int (*dcSimBegin)(void *, struct es_sim_dc *);
	void (*dcStepBegin)(void *, struct es_sim_dc *);
	void (*dcStepIter)(void *, struct es_sim_dc *);
	void (*dcStepEnd)(void *, struct es_sim_dc *);
	void (*dcSimEnd)(void *, struct es_sim_dc *);
	void (*dcUpdateError)(void *, struct es_sim_dc *, M_Real *);
	
	TAILQ_HEAD(,vg_node) schemEnts;		/* Schematic entities */
	TAILQ_ENTRY(es_component) components;
} ES_Component;

#define ESCOMPONENT(p)		((ES_Component *)(p))
#define ESCOMPONENT_CIRCUIT(p)	(ESCOMPONENT(p)->ckt)
#define ESCOMPONENT_CLASS(p)	((ES_ComponentClass *)(AGOBJECT(p)->cls))
#define ES_PORT(p,n)		(&ESCOMPONENT(p)->ports[n])
#define ES_PAIR(p,n)		(&ESCOMPONENT(p)->pairs[n])
#define ES_VPORT(p,n)		ES_NodeVoltage(ESCOMPONENT_CIRCUIT(p),\
 			        	       ES_PNODE((p),(n)))
#define ES_V_PREV_STEP(p,n)	ES_NodeVoltagePrevStep(ESCOMPONENT_CIRCUIT(p),\
 			        	       ES_PNODE((p),(n)))
#define ES_IBRANCH(p,n)		ES_BranchCurrent(ESCOMPONENT_CIRCUIT(p),\
 			        	       ES_PNODE((p),(n)))
#define ES_I_PREV_STEP(p,n)	ES_BranchCurrentPrevStep(ESCOMPONENT_CIRCUIT(p),\
 			        	       ES_PNODE((p),(n)))
#ifdef DEBUG
# define ES_PNODE(p,n)		ES_PortNode(ESCOMPONENT(p),(n))
#else
# define ES_PNODE(p,n)		(ESCOMPONENT(p)->ports[n].node)
#endif

#define ESCOMPONENT_FOREACH_PORT(port, i, com) \
	for ((i) = 1; \
	    ((i) <= (com)->nports) && ((port) = &(com)->ports[i]); \
	    (i)++)
#define ESCOMPONENT_FOREACH_PAIR(pair, i, com) \
	for ((i) = 0; \
	    ((i) < (com)->npairs) && ((pair) = &(com)->pairs[i]); \
	    (i)++)
#define ESCOMPONENT_IS_FLOATING(com) \
	(AG_ObjectIsClass((com),"ES_Component:*") && \
	 ESCOMPONENT(com)->flags & ES_COMPONENT_FLOATING)

#if defined(_ES_INTERNAL) || defined(_USE_EDACIOUS_CORE)
# define COMPONENT(p) ESCOMPONENT(p)
# define COMCIRCUIT(p) ESCOMPONENT_CIRCUIT(p)
# define COMCLASS(p) ESCOMPONENT_CLASS(p)
# define COMPONENT_FOREACH_PORT(p,i,com) ESCOMPONENT_FOREACH_PORT(p,i,com)
# define COMPONENT_FOREACH_PAIR(p,i,com) ESCOMPONENT_FOREACH_PAIR(p,i,com)
# define COMPONENT_IS_FLOATING(com)	 ESCOMPONENT_IS_FLOATING(com)
# define PORT(p,n) ES_PORT(p,n)
# define PAIR(p,n) ES_PAIR(p,n)
# define VPORT(p,n) ES_VPORT(p,n)
# define V_PREV_STEP(p,n) ES_V_PREV_STEP(p,n)
# define IBRANCH(p,n) ES_IBRANCH(p,n)
# define I_PREV_STEP(p,n) ES_I_PREV_STEP(p,n)
# define PNODE(p,n) ES_PNODE(p,n)
#endif /* _ES_INTERNAL or _USE_EDACIOUS_CORE */

__BEGIN_DECLS
extern AG_ObjectClass esComponentClass;

void	 ES_ComponentLog(void *, const char *, ...);
void	 ES_ComponentMenu(ES_Component *, VG_View *);
void	 ES_InitPorts(void *, const ES_Port *);
void	 ES_FreePorts(ES_Component *);

void           ES_AttachSchemEntity(void *, VG_Node *);
void           ES_DetachSchemEntity(void *, VG_Node *);
ES_SchemBlock *ES_LoadSchemFromFile(void *, const char *);

Uint	 ES_PortNode(ES_Component *, int);
int	 ES_PairIsInLoop(ES_Pair *, struct es_loop *, int *);
ES_Port	*ES_FindPort(void *, const char *);
__END_DECLS
