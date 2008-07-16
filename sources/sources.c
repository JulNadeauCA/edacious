/*	Public domain	*/

#include <core/core.h>
#include "sources.h"

/* Components implemented by libsources. */
void *esSourcesClasses[] = {
	&esIsourceClass,
	&esVsourceClass,
	&esVArbClass,
	&esVSineClass,
	&esVSquareClass,
	&esVSweepClass,
	&esVNoiseClass,
	NULL
};

/* Initialize libsources. */
void
ES_SourcesInit(void)
{
	void **cls;

	for (cls = &esSourcesClasses[0]; *cls != NULL; cls++)
		ES_RegisterClass(*cls);
}

void
ES_SourcesDestroy(void)
{
}
