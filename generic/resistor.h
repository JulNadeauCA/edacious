/*	Public domain	*/

typedef struct es_resistor {
	struct es_component _inherit;

	M_Real Rnom;		/* Resistance at Tnom */
	M_Real Pmax;		/* Power rating in watts */
	int tolerance;		/* Tolerance in % */
	float Tc1, Tc2;		/* Resistance/temperature coefficients */

	M_Real g;

	StampConductanceData s;
} ES_Resistor;

__BEGIN_DECLS
extern ES_ComponentClass esResistorClass;
__END_DECLS
