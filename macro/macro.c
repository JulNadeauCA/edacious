/*	Public domain	*/

#include <core/core.h>
#include "macro.h"

ES_ComponentClass *esMacroClasses[] = {
	&esAndClass,
	&esDigitalClass,
	&esInverterClass,
	&esOrClass,
	&esLogicProbeClass,
	NULL
};

ES_Module esMacroModule = {
	EDACIOUS_VERSION,
	N_("Standard component macromodels"),
	"http://edacious.hypertriton.com/",
	NULL, /* init */
	NULL, /* destroy */
	esMacroClasses,
	NULL /* vgClasses */
};
