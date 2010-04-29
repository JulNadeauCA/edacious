/*	Public domain	*/

#include <core/core.h>
#include "sources.h"

ES_ComponentClass *esSourcesClasses[] = {
	&esIsourceClass,
	&esVsourceClass,
	&esVArbClass,
	&esVSineClass,
	&esVSquareClass,
	&esVSweepClass,
	&esVNoiseClass,
	NULL
};

ES_Module esSourcesModule = {
	EDACIOUS_VERSION,
	N_("Voltage and current sources"),
	"http://edacious.org/",
	NULL, /* init */
	NULL, /* destroy */
	esSourcesClasses,
	NULL /* vgClasses */
};
