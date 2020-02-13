/*	Public domain	*/

#define ES_SCHEM_NAME_MAX 128

/* Circuit schematic block */
typedef struct es_schem {
	struct ag_object _inherit;		/* AG_Object -> ES_Schem */
	VG *vg;
	TAILQ_ENTRY(es_schem) schems;
} ES_Schem;

#define ESSCHEM(p)             ((ES_Schem *)(p))
#define ESCSCHEM(obj)          ((const ES_Schem *)(obj))
#define ES_SCHEM_SELF()          ESSCHEM( AG_OBJECT(0,"ES_Schem:*") )
#define ES_SCHEM_PTR(n)          ESSCHEM( AG_OBJECT((n),"ES_Schem:*") )
#define ES_SCHEM_NAMED(n)        ESSCHEM( AG_OBJECT_NAMED((n),"ES_Schem:*") )
#define ES_CONST_SCHEM_SELF()   ESCSCHEM( AG_CONST_OBJECT(0,"ES_Schem:*") )
#define ES_CONST_SCHEM_PTR(n)   ESCSCHEM( AG_CONST_OBJECT((n),"ES_Schem:*") )
#define ES_CONST_SCHEM_NAMED(n) ESCSCHEM( AG_CONST_OBJECT_NAMED((n),"ES_Schem:*") )

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
