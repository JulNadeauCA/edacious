/*	Public domain	*/

/* Connection point in schematic block */
typedef struct es_schem_port {
	struct vg_node _inherit;
	VG_Point *p;			/* Location point */
	VG_Text *lbl;			/* Text label */
	char name[ES_SCHEM_NAME_MAX];	/* Symbol name */
	float r;			/* Radius of circle */
} ES_SchemPort;

__BEGIN_DECLS
extern const VG_NodeOps esSchemPortOps;

static __inline__ ES_SchemPort *
ES_SchemPortNew(void *pNode, VG_Point *pCenter)
{
	ES_SchemPort *sp;
	VG_Point *p1, *p2;

	p1 = VG_PointNew(pCenter, VGVECTOR(-0.5f, 0.5f));
	p2 = VG_PointNew(pCenter, VGVECTOR(+0.5f, 0.5f));

	sp = AG_Malloc(sizeof(ES_SchemPort));
	VG_NodeInit(sp, &esSchemPortOps);
	sp->p = pCenter;

	VG_NodeAttach(pNode, sp);
	VG_AddRef(sp, pCenter);

	sp->lbl = VG_TextNew(sp, p1, p2);
	return (sp);
}
__END_DECLS
