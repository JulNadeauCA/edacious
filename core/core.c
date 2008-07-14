/*
 * Copyright (c) 2008 Hypertriton, Inc. <http://hypertriton.com/>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <edacious/eda.h>

/* Built-in component model classes. */
void *esComponentClasses[] = {
	&esGroundClass,
	&esIsourceClass,
	&esVsourceClass,
	&esVSquareClass,
	&esVSineClass,
	&esVArbClass,
	&esVSweepClass,
	&esResistorClass,
	&esSemiResistorClass,
	&esSpstClass,
	&esSpdtClass,
	&esLogicProbeClass,
	&esInverterClass,
	&esAndClass,
	&esOrClass,
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

/* Built-in schematic entity (VG) classes. */
const void *esSchematicClasses[] = {
	&esSchemBlockOps,
	&esSchemPortOps,
	&esSchemWireOps,
	NULL
};

/* Agar object classes for core Edacious functionality. */
void *esBaseClasses[] = {
	&esComponentClass,
	&esDigitalClass,
	&esWireClass,
	&esCircuitClass,
	&esScopeClass,
	&esSchemClass,
	NULL
};

/* User-creatable object classes. */
const char *esEditableClasses[] = {
	"ES_Circuit:*",
	"ES_Schem:*",
	"ES_Scope:*",
	NULL
};

#ifdef FP_DEBUG
#include <fenv.h>
#endif

#include "icons_data.h"

AG_Object esVfsRoot;			/* General-purpose VFS */

/* Initialize the Edacious library. */
void
ES_InitSubsystem(void)
{
	const void **clsSchem;
	void **cls;

	/* Initialize the VG and M libraries. */
	VG_InitSubsystem();
	M_InitSubsystem();
	
	/* Register our built-in classes. */
	for (cls = &esBaseClasses[0]; *cls != NULL; cls++)
		AG_RegisterClass(*cls);
	for (cls = &esComponentClasses[0]; *cls != NULL; cls++)
		AG_RegisterClass(*cls);
	for (clsSchem = &esSchematicClasses[0]; *clsSchem != NULL; clsSchem++)
		VG_RegisterClass(*clsSchem);

#ifdef FP_DEBUG
	/* Handle division by zero and overflow. */
	feenableexcept(FE_DIVBYZERO | FE_OVERFLOW);
#endif
	/* Load our built-in GUI icons. */
	esIcon_Init();
}
