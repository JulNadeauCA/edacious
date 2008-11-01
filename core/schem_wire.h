/*	Public domain	*/

/* Connection point in schematic block */
typedef struct es_schem_wire {
	struct vg_node _inherit;
	VG_Node *p1, *p2;		/* Endpoints */
	Uint thickness;			/* Thickness in pixels */
	void *wire;			/* Pointer to Wire component */
	char name[AG_OBJECT_NAME_MAX];	/* Wire component name (R) */
} ES_SchemWire;

#ifdef _ES_INTERNAL
#define SCHEM_WIRE(p) ((ES_SchemWire *)(p))
#endif

__BEGIN_DECLS
extern VG_NodeOps esSchemWireOps;

ES_SchemWire *ES_SchemWireNew(void *, void *, void *);
__END_DECLS
