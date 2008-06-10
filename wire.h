/*	Public domain	*/

#ifndef _COMPONENT_WIRE_H_
#define _COMPONENT_WIRE_H_

#include "component.h"

#include "begin_code.h"

typedef struct es_wire {
	struct es_component _inherit;
	Uint flags;
#define ES_WIRE_FIXED	0x01		/* Don't allow moving */
	Uint cat;			/* Category */
	ES_SchemWire *schemWire;	/* Schematic wire entity */
} ES_Wire;

__BEGIN_DECLS
extern ES_ComponentClass esWireClass;

ES_Wire *ES_WireNew(ES_Circuit *);
__END_DECLS

#include "close_code.h"
#endif /* _COMPONENT_WIRE_H_ */
