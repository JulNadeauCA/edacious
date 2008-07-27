/*	Public domain	*/

typedef struct es_spst {
	struct es_component _inherit;
	M_Real rOn;			/* ON-resistance */
	M_Real rOff;			/* OFF-resistance */

	M_Real g;			/* conductance at the beginning of the last time step */

	int state;			/* Last state */

	StampConductanceData s;
} ES_Spst;

__BEGIN_DECLS
extern ES_ComponentClass esSpstClass;
__END_DECLS
