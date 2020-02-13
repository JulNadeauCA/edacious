/*	Public domain	*/

struct es_circuit;

/* PCB layout or layout block */
typedef struct es_layout {
	struct ag_object _inherit;		/* AG_Object -> ES_Layout */
	struct es_circuit *ckt;			/* Associated circuit */
	VG *vg;					/* Graphical layout */
	TAILQ_ENTRY(es_layout) layouts;
	TAILQ_HEAD(,es_package) packages;	/* Device packages */
} ES_Layout;

#define ESLAYOUT(p)             ((ES_Layout *)(p))
#define ESCLAYOUT(obj)          ((const ES_Layout *)(obj))
#define ES_LAYOUT_SELF()          ESLAYOUT( AG_OBJECT(0,"ES_Layout:*") )
#define ES_LAYOUT_PTR(n)          ESLAYOUT( AG_OBJECT((n),"ES_Layout:*") )
#define ES_LAYOUT_NAMED(n)        ESLAYOUT( AG_OBJECT_NAMED((n),"ES_Layout:*") )
#define ES_CONST_LAYOUT_SELF()   ESCLAYOUT( AG_CONST_OBJECT(0,"ES_Layout:*") )
#define ES_CONST_LAYOUT_PTR(n)   ESCLAYOUT( AG_CONST_OBJECT((n),"ES_Layout:*") )
#define ES_CONST_LAYOUT_NAMED(n) ESCLAYOUT( AG_CONST_OBJECT_NAMED((n),"ES_Layout:*") )

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
