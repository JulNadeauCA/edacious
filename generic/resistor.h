/*	Public domain	*/

typedef struct es_resistor {
	struct es_component _inherit;
	M_Real R;			/* Resistance at Tnom */
	M_Real Pmax;			/* Power rating (W) */
	M_Real Tc1, Tc2;		/* 1st and 2nd order temperature coefficients */
	M_Real g;			/* Last computed conductance */
	StampConductanceData s;
} ES_Resistor;

__BEGIN_DECLS
extern ES_ComponentClass esResistorClass;
__END_DECLS
