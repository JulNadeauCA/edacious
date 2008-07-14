/*	Public domain	*/

typedef struct es_led {
	struct es_component _inherit;

	M_Real Vforw;		/* Forward voltage */
	M_Real Vrev;		/* Reverse voltage */
	M_Real I;		/* Luminous intensity (mcd) */
	int state;		/* Last state */
} ES_Led;

__BEGIN_DECLS
extern ES_ComponentClass esLedClass;
__END_DECLS
