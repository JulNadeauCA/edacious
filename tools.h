/* Circuit edition tools */
static VG_ToolOps *esCircuitTools[] = {
	&esSchemSelectTool,
	&esWireTool,
	NULL
};

/* Circuit schematic edition tools */
static VG_ToolOps *esCircuitSchemTools[] = {
	&esSchemPointTool,
	&esSchemLineTool,
	&esSchemCircleTool,
	&esSchemTextTool,
#ifdef DEBUG
	&esSchemProximityTool,
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
	&esSchemPointTool,
	&esSchemLineTool,
	&esSchemCircleTool,
	&esSchemTextTool,
#ifdef DEBUG
	&esSchemProximityTool,
#endif
	NULL
};
