/*	Public domain	*/

/* Connection point in PCB layout (analogous to SchemPort) */
typedef struct es_layout_node {
	struct vg_node _inherit;
	Uint flags;
} ES_LayoutNode;

#ifdef _ES_INTERNAL
#define LAYOUT_NODE(p) ((ES_LayoutNode *)(p))
#endif

__BEGIN_DECLS
extern VG_NodeOps esLayoutNodeOps;

ES_LayoutNode *ES_LayoutNodeNew(void *);
__END_DECLS
