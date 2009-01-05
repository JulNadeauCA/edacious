/*	Public domain	*/

/* PCB layout or layout block */
typedef struct es_layout {
	struct ag_object _inherit;
	VG *vg;
	TAILQ_ENTRY(es_layout) layouts;
} ES_Layout;

#ifdef _ES_INTERNAL
#define LAYOUT(p) ((ES_Layout *)(p))
#endif

#include <edacious/core/layout_node.h>
#include <edacious/core/layout_trace.h>
#include <edacious/core/layout_block.h>

__BEGIN_DECLS
extern AG_ObjectClass esLayoutClass;

extern VG_ToolOps esLayoutSelectTool;
extern VG_ToolOps esLayoutNodeTool;
extern VG_ToolOps esLayoutTraceTool;

ES_Layout *ES_LayoutNew(void *);
void      *ES_LayoutNearest(VG_View *, VG_Vector);
__END_DECLS
