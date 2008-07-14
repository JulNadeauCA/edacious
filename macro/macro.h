/*	Public domain	*/

#include <edacious/core/core_begin.h>

#include <edacious/macro/digital.h>
#include <edacious/macro/and_gate.h>
#include <edacious/macro/inverter.h>
#include <edacious/macro/or_gate.h>
#include <edacious/macro/logic_probe.h>

__BEGIN_DECLS
extern void *esMacroClasses[];
void ES_MacroInit(void);
void ES_MacroDestroy(void);
__END_DECLS

#include <edacious/core/core_close.h>
