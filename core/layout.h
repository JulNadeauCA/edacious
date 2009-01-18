/*	Public domain	*/

struct es_circuit;

/* PCB layout or layout block */
typedef struct es_layout {
	struct ag_object _inherit;
	struct es_circuit *ckt;			/* Associated circuit */
	VG *vg;					/* Graphical layout */
	TAILQ_ENTRY(es_layout) layouts;
	TAILQ_HEAD(,es_package) packages;	/* Device packages */
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
extern VG_ToolOps esPackageInsertTool;

ES_Layout *ES_LayoutNew(struct es_circuit *);
void      *ES_LayoutNearest(VG_View *, VG_Vector);
void      *ES_LayoutEdit(void *);
__END_DECLS
