/*	Public domain	*/

struct es_component;

typedef struct es_schem_block {
	struct vg_node _inherit;
	char name[AG_OBJECT_NAME_MAX];		/* Name of component (R) */
	struct es_component *com;		/* Pointer to component */
} ES_SchemBlock;

#ifdef _ES_INTERNAL
#define SCHEM_BLOCK(p) ((ES_SchemBlock *)(p))
#endif

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
