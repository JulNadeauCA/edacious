/*	Public domain	*/

#include <core/core.h>
#include "macro.h"

ES_ComponentClass *esMacroClasses[] = {
	&esDigitalClass,
	&esLogicProbeClass,
	&esAndClass,
	&esOrClass,
	&esInverterClass,
	NULL
};

ES_Module esMacroModule = {
	EDACIOUS_VERSION,
	N_("Standard component macromodels"),
	"http://edacious.org/",
	NULL, /* init */
	NULL, /* destroy */
	esMacroClasses,
	NULL /* vgClasses */
};
