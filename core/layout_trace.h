/*	Public domain	*/

/* Conductive line between two points in a PCB layout. */
typedef struct es_layout_trace {
	struct vg_node _inherit;
	Uint flags;
#define ES_LAYOUTTRACE_RAT	0x01	/* Line is a rat line and not a copper trace */
	VG_Node *p1, *p2;		/* Endpoints (LayoutNodes) */
	float thickness;		/* Width of the line */
	float clearance;		/* Space cleared around the line when passing through a polygon */
} ES_LayoutTrace;

#ifdef _ES_INTERNAL
#define LAYOUT_TRACE(p) ((ES_LayoutTrace *)(p))
#endif

__BEGIN_DECLS
extern VG_NodeOps esLayoutTraceOps;
ES_LayoutTrace *ES_LayoutTraceNew(void *, void *, void *);
__END_DECLS
