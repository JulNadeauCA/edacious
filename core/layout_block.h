/*	Public domain	*/

struct es_component;

typedef struct es_layout_block {
	struct vg_node _inherit;
	char name[AG_OBJECT_NAME_MAX];		/* Name of component (R) */
	struct es_component *com;		/* Pointer to component */
} ES_LayoutBlock;

#ifdef _ES_INTERNAL
#define LAYOUT_BLOCK(p) ((ES_LayoutBlock *)(p))
#endif

__BEGIN_DECLS
extern VG_NodeOps esLayoutBlockOps;

static __inline__ ES_LayoutBlock *
ES_LayoutBlockNew(void *pNode, const char *name)
{
	ES_LayoutBlock *lb;

	lb = AG_Malloc(sizeof(ES_LayoutBlock));
	VG_NodeInit(lb, &esLayoutBlockOps);
	Strlcpy(lb->name, name, sizeof(lb->name));
	VG_NodeAttach(pNode, lb);
	return (lb);
}

int ES_LayoutBlockLoad(ES_LayoutBlock *, const char *);
__END_DECLS
