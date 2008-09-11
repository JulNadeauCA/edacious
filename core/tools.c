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

/* VG edition tools */
VG_ToolOps *esVgTools[] = {
	&vgPointTool,
	&vgLineTool,
	&vgCircleTool,
	&vgTextTool,
	&vgPolygonTool,
#ifdef DEBUG
	&vgProximityTool,
#endif
	NULL
};
