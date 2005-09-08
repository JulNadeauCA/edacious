/*	$Csoft: tools.c,v 1.2 2004/09/02 06:13:25 vedge Exp $	*/
/*	Public domain	*/

#include <engine/engine.h>

#include "tool.h"

extern const struct eda_tool power_tool;
extern const struct eda_tool resistance_tool;
extern const struct eda_tool current_tool;
extern const struct eda_tool voltage_tool;
extern const struct eda_tool rmsvoltage_tool;
extern const struct eda_tool capacitance_tool;
extern const struct eda_tool weight_tool;
extern const struct eda_tool length_tool;
extern const struct eda_tool pressure_tool;
extern const struct eda_tool time_tool;
extern const struct eda_tool impedance_tool;
extern const struct eda_tool avreg_tool;
extern const struct eda_tool lm555_tool;

const struct eda_tool *tools[] = {
	&power_tool,
	&resistance_tool,
	&current_tool,
	&voltage_tool,
	&rmsvoltage_tool,
	&impedance_tool,
	&capacitance_tool,
	&weight_tool,
	&length_tool,
	&pressure_tool,
	&time_tool,
	&avreg_tool,
	&lm555_tool
};

const int ntools = sizeof(tools) / sizeof(tools[0]);

