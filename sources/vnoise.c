/*
 * Copyright (c) 2005-2008 Hypertriton, Inc. <http://hypertriton.com/>
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
 * Noisy voltage source.
 */

#include <core/core.h>
#include "sources.h"
#include <string.h>
#include <errno.h>

const ES_Port esVNoisePorts[] = {
	{  0, "" },
	{  1, "v+" },
	{  2, "v-" },
	{ -1 },
};

static int
DC_SimBegin(void *obj, ES_SimDC *dc)
{
	ES_VNoise *vn = obj;
	Uint k = PNODE(vn,1);
	Uint j = PNODE(vn,2);

	/* Open random source and fill buffer. */
	if ((vn->srcFd = fopen(vn->srcPath, "rb")) == NULL) {
		AG_SetError("%s: %s", vn->srcPath, strerror(errno));
		return (-1);
	}
	fread(vn->buf, vn->bufSize, 1, vn->srcFd);
	vn->bufPos = 0;
	
	VSOURCE(vn)->v = 0.0;
	InitStampVoltageSource(k,j, VSOURCE(vn)->vIdx, VSOURCE(vn)->s, dc);
	StampVoltageSource(VSOURCE(vn)->v, VSOURCE(vn)->s);
	return (0);
}

static void
DC_SimEnd(void *obj, ES_SimDC *dc)
{
	ES_VNoise *vn = obj;

	if (vn->srcFd != NULL) {
		fclose(vn->srcFd);
		vn->srcFd = NULL;
	}
}

static void
RefillBuffer(ES_VNoise *vn)
{
	Uint32 bufNew[1024];
	int i;

	fread(bufNew, vn->bufSize, 1, vn->srcFd);
	for (i = 0; i < vn->bufSize/sizeof(Uint32); i++) {
		vn->buf[i] *= bufNew[i];
	}
	vn->bufPos = 0;
}

static void
DC_StepBegin(void *obj, ES_SimDC *dc)
{
	ES_VNoise *vn = obj;
	Uint32 r;
	M_Real v;

	if (++vn->bufPos > vn->bufSize/sizeof(Uint32)) {
		RefillBuffer(vn);
	}
	r = vn->buf[vn->bufPos];
	r = (r << 13) ^ r;
	v = vn->vMin +
	    vn->vMax*10.0*( (r*(r*r*15731 + 789221) + 1376312589) & 0x7ffffff) /
	    1073741824.0;
	if (Fabs(v - vn->vPrev) > vn->deltaMax) {
		v = (v > vn->vPrev) ? vn->vPrev+vn->deltaMax :
		                      vn->vPrev-vn->deltaMax;
	}
	VSOURCE(vn)->v = v;

	StampVoltageSource(VSOURCE(vn)->v, VSOURCE(vn)->s);
	vn->vPrev = VSOURCE(vn)->v;
}

static void
DC_StepIter(void *obj, ES_SimDC *dc)
{
	ES_VNoise *vn = obj;
	StampVoltageSource(VSOURCE(vn)->v, VSOURCE(vn)->s);
}
static void
Init(void *p)
{
	ES_VNoise *vn = p;

	ES_InitPorts(vn, esVNoisePorts);

	vn->vPrev = 0.0;
	vn->vMin = 0.0;
	vn->vMax = 5.0;
	vn->srcFd = NULL;
	vn->bufSize = 4096;
	vn->buf = Malloc(vn->bufSize);
	vn->bufPos = 0;
	vn->deltaMax = 0.1;

	Strlcpy(vn->srcPath, "/dev/urandom", sizeof(vn->srcPath));

	COMPONENT(vn)->dcSimBegin = DC_SimBegin;
	COMPONENT(vn)->dcSimEnd = DC_SimEnd;
	COMPONENT(vn)->dcStepBegin = DC_StepBegin;
	COMPONENT(vn)->dcStepIter = DC_StepIter;
}

static void
Destroy(void *p)
{
	ES_VNoise *vn = p;

	Free(vn->buf);
}

static int
Load(void *p, AG_DataSource *ds, const AG_Version *ver)
{
	ES_VNoise *vn= p;

	vn->vMin = M_ReadReal(ds);
	vn->vMax = M_ReadReal(ds);
	vn->deltaMax = M_ReadReal(ds);
	AG_CopyString(vn->srcPath, ds, sizeof(vn->srcPath));
	return (0);
}

static int
Save(void *p, AG_DataSource *ds)
{
	ES_VNoise *vn = p;

	M_WriteReal(ds, vn->vMin);
	M_WriteReal(ds, vn->vMax);
	M_WriteReal(ds, vn->deltaMax);
	AG_WriteString(ds, vn->srcPath);
	return (0);
}

static void *
Edit(void *p)
{
	ES_VNoise *vn = p;
	AG_Box *box = AG_BoxNewVert(NULL, AG_BOX_EXPAND);
	AG_Textbox *tb;

	tb = AG_TextboxNew(box, 0, _("Random source: "));
	AG_TextboxBindASCII(tb, vn->srcPath, sizeof(vn->srcPath));

	M_NumericalNewReal(box, 0, "V", _("Min. voltage: "), &vn->vMin);
	M_NumericalNewReal(box, 0, "V", _("Max. voltage: "), &vn->vMax);
	M_NumericalNewRealR(box, 0, "V", _("Max. delta: "), &vn->deltaMax,
	    M_TINYVAL, M_HUGEVAL);
	return (box);
}

ES_ComponentClass esVNoiseClass = {
	{
		"ES_Component:ES_Vsource:ES_VNoise",
		sizeof(ES_VNoise),
		{ 0,0 },
		Init,
		NULL,		/* reinit */
		Destroy,
		Load,
		Save,
		Edit
	},
	N_("Voltage source (noise)"),
	"Vnoise",
	"Sources/Vnoise.eschem",
	"Generic|Sources",
	&esIconVsine,
	NULL,			/* draw */
	NULL,			/* instance_menu */
	NULL,			/* class_menu */
	NULL,			/* export */
	NULL			/* connect */
};
