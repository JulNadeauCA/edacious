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
