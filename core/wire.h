/*	Public domain	*/

typedef struct es_wire {
	struct es_component _inherit;
	Uint flags;
#define ES_WIRE_FIXED	0x01		/* Don't allow moving */
	Uint cat;			/* Hint for Layout */
} ES_Wire;

#ifdef _ES_INTERNAL
#define WIRE(p) ((ES_Wire *)(p))
#endif

__BEGIN_DECLS
extern ES_ComponentClass esWireClass;

ES_Wire *ES_WireNew(ES_Circuit *);
__END_DECLS
