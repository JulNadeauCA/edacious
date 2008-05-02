/*	Public domain	*/

#define ES_SCHEM_SYM_MAX 16

/* Connection point in schematic block */
typedef struct es_schem_port {
	struct vg_node _inherit;
	VG_Point *p;			/* Location point */
	VG_Text *lbl;			/* Text label */
	Uint n;				/* Port number (0 = use symbol) */
	char sym[ES_SCHEM_SYM_MAX];	/* Symbol name */
	float r;			/* Radius of circle */
} ES_SchemPort;

__BEGIN_DECLS
extern const VG_NodeOps esSchemPortOps;

static __inline__ ES_SchemPort *
ES_SchemPortNew(void *pNode, VG_Point *pCenter)
{
	ES_SchemPort *sp;
	VG_Point *p1, *p2;

	p1 = VG_PointNew(pCenter, VGVECTOR(0.0f, 0.0f));
	p2 = VG_PointNew(pCenter, VGVECTOR(0.5f, 0.0f));

	sp = AG_Malloc(sizeof(ES_SchemPort));
	VG_NodeInit(sp, &esSchemPortOps);
	sp->p = pCenter;
	VG_NodeAttach(pNode, sp);
	VG_AddRef(sp, pCenter);

	sp->lbl = VG_TextNew(sp, p1, p2);
	VG_TextPrintfPolled(sp->lbl, "Port%u", &sp->n);
	return (sp);
}
__END_DECLS
