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
 * Line tool.
 */

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>

#include <eda.h>
#include "schem.h"

typedef struct es_schem_line_tool {
	VG_Tool _inherit;
	VG_Node *curLine;
} ES_SchemLineTool;

static void
Init(void *p)
{
	ES_SchemLineTool *t = p;

	t->curLine = NULL;
}

static int
MouseButtonDown(void *p, float x, float y, int b)
{
	ES_SchemLineTool *t = p;
	ES_Schem *scm = VGTOOL(t)->p;
	VG_View *vv = VGTOOL(t)->vgv;
	VG *vg = scm->vg;

	switch (b) {
	case SDL_BUTTON_LEFT:
		if (t->curLine == NULL) {
			VG_Status(vv, _("New line at %.2f,%.2f"), x, y);
			t->curLine = VG_Begin(vg, VG_LINES);
			VG_Vertex2(vg, x, y);
			VG_Vertex2(vg, x, y);
			VG_End(vg);
		} else {
			VG_Select(vg, t->curLine);
			VG_MoveVertex2(vg, 1, x, y);
			VG_End(vg);
			VG_Status(vv, _("Added line: %.2f,%.2f -> %.2f,%.2f"),
			    t->curLine->vtx[0].x, t->curLine->vtx[0].y,
			    t->curLine->vtx[1].x, t->curLine->vtx[1].y);
			t->curLine = NULL;
		}
		return (1);
	case SDL_BUTTON_MIDDLE:
	case SDL_BUTTON_RIGHT:
		if (t->curLine != NULL) {
			VG_Delete(vg, t->curLine);
			t->curLine = NULL;
		}
		return (1);
	default:
		return (0);
	}
}

static int
MouseMotion(void *p, float x, float y, float xrel, float yrel, int b)
{
	ES_SchemLineTool *t = p;
	VG_View *vv = VGTOOL(t)->vgv;
	
	if (t->curLine != NULL) {
		ES_Schem *scm = VGTOOL(t)->p;
		VG *vg = scm->vg;
	
		VG_Status(vv, _("Line endpoint at %f,%f"), x, y);
		VG_Select(vg, t->curLine);
		VG_MoveVertex2(vg, 1, x, y);
		VG_End(vg);
	}
	return (0);
}

VG_ToolOps esSchemLineTool = {
	N_("Line"),
	N_("Insert lines in the component schematic."),
	&vgIconLine,
	sizeof(ES_SchemLineTool),
	0,
	Init,
	NULL,			/* destroy */
	NULL,			/* edit */
	MouseMotion,
	MouseButtonDown,
	NULL,			/* mousebuttonup */
	NULL,			/* keydown */
	NULL			/* keyup */
};
