/*	Public domain	*/

#include <core/core.h>
#include "generic.h"

ES_ComponentClass *esGenericClasses[] = {
	&esGroundClass,
	&esResistorClass,
	&esSemiResistorClass,
	&esSpstClass,
	&esSpdtClass,
	&esDiodeClass,
	&esLedClass,
	&esNMOSClass, 
	&esPMOSClass,
	&esCapacitorClass,
	&esInductorClass,
	&esNPNClass,
	&esPNPClass,
	NULL
};

ES_Module esGenericModule = {
	EDACIOUS_VERSION,
	N_("Standard generic components"),
	"http://edacious.org/",
	NULL, /* init */
	NULL, /* destroy */
	esGenericClasses,
	NULL /* vgClasses */
};
