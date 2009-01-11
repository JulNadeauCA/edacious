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

/*
 * Wire component.
 */

#include "core.h"

const ES_Port esWirePorts[] = {
	{ 0, "" },
	{ 1, "A" },
	{ 2, "B" },
	{ -1 },
};

ES_Wire *
ES_WireNew(ES_Circuit *ckt)
{
	char name[AG_OBJECT_NAME_MAX];
	ES_Wire *wire;

	/* Allocate the wire object instance. */
	wire = Malloc(sizeof(ES_Wire));
	AG_ObjectInit(wire, &esWireClass);
	AG_ObjectGenNamePfx(ckt, esWireClass.pfx, name, sizeof(name));
	AG_ObjectSetName(wire, "%s", name);

	/* Attach the wire object to the circuit as a floating component. */
	ES_LockCircuit(ckt);
	AG_ObjectAttach(ckt, wire);
	AG_PostEvent(ckt, wire, "circuit-shown", NULL);
	ES_UnlockCircuit(ckt);

	return (wire);
}

static int
Load(void *p, AG_DataSource *buf, const AG_Version *ver)
{
	ES_Wire *w = p;

	w->flags = (Uint)AG_ReadUint32(buf);
	w->cat = (Uint)AG_ReadUint32(buf);
	return (0);
}

static int
Save(void *p, AG_DataSource *buf)
{
	ES_Wire *w = p;

	AG_WriteUint32(buf, (Uint32)w->flags);
	AG_WriteUint32(buf, (Uint32)w->cat);
	return (0);
}

static void
Init(void *p)
{
	ES_Wire *w = p;

	ES_InitPorts(w, esWirePorts);
	w->flags = 0;
	w->cat = 0;
	w->schemWire = NULL;

	/* Exclude from displayed component lists */
	COMPONENT(w)->flags |= ES_COMPONENT_SPECIAL;
}

static void *
Edit(void *p)
{
	ES_Wire *w = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);

	AG_CheckboxNewFlag(box, 0, _("Fixed"),
	    &w->flags, ES_WIRE_FIXED);
	return (box);
}

ES_ComponentClass esWireClass = {
	{
		"Edacious(Circuit:Component:Wire)",
		sizeof(ES_Wire),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		NULL,		/* destroy */
		Load,
		Save,
		Edit
	},
	N_("Wire"),
	"W",
	"Generic",
	&esIconInsertWire,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
