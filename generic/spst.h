/*	Public domain	*/

typedef struct es_spst {
	struct es_component _inherit;
	M_Real Ron;			/* ON-resistance */
	M_Real Roff;			/* OFF-resistance */
	M_Real g;			/* Conductance at the beginning of the last time step */
	int state;			/* Last state */
	StampConductanceData s;
} ES_Spst;

__BEGIN_DECLS
extern ES_ComponentClass esSpstClass;
__END_DECLS
