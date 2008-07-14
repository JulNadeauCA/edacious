/*	Public domain	*/

#include <core/core.h>
#include "generic.h"

/* Components implemented by libgeneric. */
void *esGenericClasses[] = {
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

/* Initialize libgeneric. */
void
ES_GenericInit(void)
{
	void **cls;

	for (cls = &esGenericClasses[0]; *cls != NULL; cls++)
		ES_RegisterClass(*cls);
}

void
ES_GenericDestroy(void)
{
}
