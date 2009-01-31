/*	Public domain	*/

typedef struct es_spdt {
	struct es_component _inherit;
	M_Real Ron;			/* ON-resistance */
	M_Real Roff;			/* OFF-resistance */
	int state;			/* Last state */
} ES_Spdt;

__BEGIN_DECLS
extern ES_ComponentClass esSpdtClass;
__END_DECLS
