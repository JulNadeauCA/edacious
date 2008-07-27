/*	Public domain	*/

typedef struct es_semiresistor {
	struct es_component _inherit;
	
	M_Real l, w;		/* Semiconductor length and width (m) */
	M_Real rSh;		/* Resistance (ohms/sq) */
	M_Real narrow;		/* Narrowing due to side etching */
	M_Real Tc1, Tc2;	/* Resistance/temperature coefficients */
	M_Real rEff;		/* Effective resistance */

	StampConductanceData s;
} ES_SemiResistor;

__BEGIN_DECLS
extern ES_ComponentClass esSemiResistorClass;
__END_DECLS
