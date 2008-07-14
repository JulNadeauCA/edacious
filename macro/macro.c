/*	Public domain	*/

#include <core/core.h>
#include "macro.h"

/* Components implemented by libmacro. */
void *esMacroClasses[] = {
	&esAndClass,
	&esDigitalClass,
	&esInverterClass,
	&esOrClass,
	&esLogicProbeClass,
	NULL
};

/* Initialize libmacro. */
void
ES_MacroInit(void)
{
	void **cls;

	for (cls = &esMacroClasses[0]; *cls != NULL; cls++)
		ES_RegisterClass(*cls);
}

void
ES_MacroDestroy(void)
{
}
