/*	Public domain	*/

struct es_port;

/* Connection point in schematic block */
typedef struct es_schem_port {
	struct vg_node _inherit;
	Uint flags;
	char name[ES_SCHEM_NAME_MAX];		/* Symbol name */
	float r;				/* Circle radius */
	char comName[AG_OBJECT_NAME_MAX];	/* Name of component (R) */
	struct es_port *port;			/* Pointer to component port */
	int portName;				/* Component port number (R) */
} ES_SchemPort;

#ifdef _ES_INTERNAL
#define SCHEM_PORT(p) ((ES_SchemPort *)(p))
#endif

__BEGIN_DECLS
extern VG_NodeOps esSchemPortOps;

ES_SchemPort *ES_SchemPortNew(void *, struct es_port *);
__END_DECLS
