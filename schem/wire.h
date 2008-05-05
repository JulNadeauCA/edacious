/*	Public domain	*/

/* Connection point in schematic block */
typedef struct es_schem_wire {
	struct vg_node _inherit;
	VG_Point *p1, *p2;		/* Endpoints */
	Uint thickness;			/* Thickness in pixels */
} ES_SchemWire;

__BEGIN_DECLS
extern const VG_NodeOps esSchemWireOps;

static __inline__ ES_SchemWire *
ES_SchemWireNew(void *pNode, VG_Point *p1, VG_Point *p2)
{
	ES_SchemWire *sw;

	sw = AG_Malloc(sizeof(ES_SchemWire));
	VG_NodeInit(sw, &esSchemWireOps);
	sw->p1 = p1;
	sw->p2 = p2;

	VG_NodeAttach(pNode, sw);
	VG_AddRef(sw, p1);
	VG_AddRef(sw, p2);
	return (sw);
}
__END_DECLS
