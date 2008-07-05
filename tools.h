/* Circuit edition tools */
static VG_ToolOps *esCircuitTools[] = {
	&esSchemSelectTool,
	&esWireTool,
	NULL
};

/* Circuit schematic edition tools */
static VG_ToolOps *esCircuitSchemTools[] = {
	&vgPointTool,
	&vgLineTool,
	&vgCircleTool,
	&vgTextTool,
#ifdef DEBUG
	&vgProximityTool,
#endif
	NULL
};

/* Component schematic edition tools */
static VG_ToolOps *esSchemTools[] = {
	&esSchemSelectTool,
	&esSchemPortTool,
	NULL
};
static VG_ToolOps *esSchemVGTools[] = {
	&vgPointTool,
	&vgLineTool,
	&vgCircleTool,
	&vgTextTool,
#ifdef DEBUG
	&vgProximityTool,
#endif
	NULL
};
