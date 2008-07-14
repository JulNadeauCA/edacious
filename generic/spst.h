/*	Public domain	*/

typedef struct es_spst {
	struct es_component _inherit;
	M_Real rOn;			/* ON-resistance */
	M_Real rOff;			/* OFF-resistance */
	int state;			/* Last state */
} ES_Spst;

__BEGIN_DECLS
extern ES_ComponentClass esSpstClass;
__END_DECLS
