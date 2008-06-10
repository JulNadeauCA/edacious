/*	Public domain	*/

/* Connection point in schematic block */
typedef struct es_schem_wire {
	struct vg_node _inherit;
	VG_Node *p1, *p2;		/* Endpoints */
	Uint thickness;			/* Thickness in pixels */
	void *wire;			/* Back pointer to Wire component */
} ES_SchemWire;

__BEGIN_DECLS
extern const VG_NodeOps esSchemWireOps;

ES_SchemWire *ES_SchemWireNew(void *, void *, void *);
__END_DECLS
