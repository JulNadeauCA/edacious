/*	Public domain	*/

#define ES_SCHEM_NAME_MAX 128

/* Circuit schematic block */
typedef struct es_schem {
	struct ag_object _inherit;
	VG *vg;
	TAILQ_ENTRY(es_schem) schems;
} ES_Schem;

#ifdef _ES_INTERNAL
#define SCHEM(p) ((ES_Schem *)(p))
#endif

#include <edacious/core/schem_port.h>
#include <edacious/core/schem_wire.h>
#include <edacious/core/schem_block.h>

__BEGIN_DECLS
extern AG_ObjectClass esSchemClass;

extern VG_ToolOps esSchemSelectTool;
extern VG_ToolOps esSchemPortTool;

ES_Schem *ES_SchemNew(void *);
void     *ES_SchemNearest(VG_View *, VG_Vector);
void     *ES_SchemEdit(void *);
__END_DECLS
