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
