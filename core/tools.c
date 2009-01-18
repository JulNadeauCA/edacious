/*	Public domain	*/

#include "core.h"

/* Circuit edition tools */
VG_ToolOps *esCircuitTools[] = {
	&esSchemSelectTool,
	&esWireTool,
	NULL
};

/* Component schematic edition tools */
VG_ToolOps *esSchemTools[] = {
	&esSchemSelectTool,
	&esSchemPortTool,
	NULL
};

/* PCB layout edition tools */
VG_ToolOps *esLayoutTools[] = {
	&esLayoutSelectTool,
	&esLayoutNodeTool,
	&esLayoutTraceTool,
	NULL
};

/* VG edition tools */
VG_ToolOps *esVgTools[] = {
	&vgSelectTool,
	&vgPointTool,
	&vgLineTool,
	&vgCircleTool,
	&vgTextTool,
	&vgPolygonTool,
#ifdef ES_DEBUG
	&vgProximityTool,
#endif
	NULL
};
