/*	Public domain	*/

struct es_component;

typedef struct es_schem_block {
	struct vg_node _inherit;
	char name[ES_SCHEM_NAME_MAX];		/* Block name */
	struct es_component *com;		/* Back pointer to component */
} ES_SchemBlock;

__BEGIN_DECLS
extern const VG_NodeOps esSchemBlockOps;

static __inline__ ES_SchemBlock *
ES_SchemBlockNew(void *pNode, const char *name)
{
	ES_SchemBlock *sb;

	sb = AG_Malloc(sizeof(ES_SchemBlock));
	VG_NodeInit(sb, &esSchemBlockOps);
	Strlcpy(sb->name, name, sizeof(sb->name));
	VG_NodeAttach(pNode, sb);
	return (sb);
}

int ES_SchemBlockLoad(ES_SchemBlock *, const char *);
__END_DECLS
