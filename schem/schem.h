/*	Public domain	*/

#include "begin_code.h"

typedef struct es_schem {
	struct ag_object _inherit;
	VG *vg;
} ES_Schem;

#define SCHEM(p) ((ES_Schem *)(p))

__BEGIN_DECLS
extern AG_ObjectClass esSchemClass;
extern VG_ToolOps esSchemSelectTool;
extern VG_ToolOps esSchemLineTool;

ES_Schem *ES_SchemNew(void *);
__END_DECLS

#include "close_code.h"
