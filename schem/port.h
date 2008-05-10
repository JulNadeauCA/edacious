/*	Public domain	*/

/* Connection point in schematic block */
typedef struct es_schem_port {
	struct vg_node _inherit;
	VG_Point *p;			/* Location point */
	VG_Text *lbl;			/* Text label */
	char name[ES_SCHEM_NAME_MAX];	/* Symbol name */
	float r;			/* Radius of circle */
	void *com;			/* Pointer to component */
	void *port;			/* Pointer to component port */
} ES_SchemPort;

__BEGIN_DECLS
extern const VG_NodeOps esSchemPortOps;

ES_SchemPort *ES_SchemPortNew(void *, VG_Point *);
__END_DECLS
