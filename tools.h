/* Circuit edition tools */
static VG_ToolOps *esCircuitTools[] = {
	&esSelectTool,
	&esWireTool,

	&esSchemSelectTool,
#ifdef DEBUG
	&esSchemProximityTool,
#endif
	&esSchemPointTool,
	&esSchemLineTool,
	&esSchemCircleTool,
	&esSchemTextTool,
	NULL
};

/* Component schematic edition tools */
static VG_ToolOps *esSchemTools[] = {
	&esSchemSelectTool,
	&esSchemPortTool,
	NULL
};
static VG_ToolOps *esSchemVGTools[] = {
	&esSchemPointTool,
	&esSchemLineTool,
	&esSchemCircleTool,
	&esSchemTextTool,
#ifdef DEBUG
	&esSchemProximityTool,
#endif
	NULL
};
