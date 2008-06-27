/*	Public domain	*/

struct es_port;

/* Connection point in schematic block */
typedef struct es_schem_port {
	struct vg_node _inherit;
	Uint flags;
	char name[ES_SCHEM_NAME_MAX];	/* Symbol name */
	float r;			/* Circle radius */
	struct es_component *com;	/* Component object */
	struct es_port *port;		/* Component port */
} ES_SchemPort;

#ifdef _ES_INTERNAL
#define SCHEM_PORT(p) ((ES_SchemPort *)(p))
#endif

__BEGIN_DECLS
extern const VG_NodeOps esSchemPortOps;

ES_SchemPort *ES_SchemPortNew(void *);
__END_DECLS
