/*	Public domain	*/

typedef struct es_capacitor {
	struct es_component _inherit;

	int vIdx;			/* Index into e */
	M_Real C;			/* Capacitance (farads) */
	M_Real V0;			/* Starting voltage */
	M_Real r, v;			/* Parameters of Thevenin model */
	StampTheveninData s;
} ES_Capacitor;

__BEGIN_DECLS
extern ES_ComponentClass esCapacitorClass;
__END_DECLS
