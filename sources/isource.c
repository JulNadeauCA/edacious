/*
 * Copyright (c) 2008 
 *
 * Antoine Levitt (smeuuh@gmail.com)
 * Steven Herbst (herbst@mit.edu)
 *
 * Hypertriton, Inc. <http://hypertriton.com/>
 *
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

/*
 * Independent current source.
 */

#include <core/core.h>
#include "sources.h"

const ES_Port esIsourcePorts[] = {
	{ 0, "" },
	{ 1, "i+" },	
	{ 2, "i-" },
	{ -1 },
};

static void
Stamp(ES_Isource *i, ES_SimDC *dc)
{
	StampCurrentSource(i->I, i->s);
}

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
        ES_Isource *i = obj;

	Uint k = PNODE(i,1);
	Uint j = PNODE(i,2);

	InitStampCurrentSource(k, j, i->s, dc);
	
	Stamp(i,dc);

	return (0);
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_Isource *i = obj;

	Stamp(i, dc);
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_Isource *i = obj;

	Stamp(i, dc);
}

static void
Init(void *p)
{
	ES_Isource *i = p;
	ES_InitPorts(i, esIsourcePorts);
	
	i->I = 1.0;

	COMPONENT(i)->dcSimBegin = DC_SimBegin;
	COMPONENT(i)->dcStepBegin = DC_StepBegin;
	COMPONENT(i)->dcStepIter = DC_StepIter;
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Isource *i = p;

	i->I = M_ReadReal(buf);
	
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Isource *i = p;

	M_WriteReal(buf, i->I);

	return (0);
}

static void *
Edit(void *p)
{
	ES_Isource *i = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	M_NumericalNewRealPNZ(box, 0, "A", _("Current: "), &i->I);

	return (box);
}

ES_ComponentClass esIsourceClass = {
	{
		"Edacious(Circuit:Component:Isource)"
		"@sources",
		sizeof(ES_Isource),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,		/* load */
		Save,		/* save */
		Edit
	},
	N_("Current source"),
	"I",
	"Generic|Sources",
	&esIconISource,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
