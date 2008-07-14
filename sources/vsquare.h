/*	Public domain	*/

typedef struct es_vsquare {
	struct es_vsource _inherit;
	M_Real vH;			/* Voltage of high signal */
	M_Real vL;			/* Voltage of low signal */
	M_Real tH;			/* High pulse duration (s)*/
	M_Real tL;			/* Low pulse duration (s)*/
} ES_VSquare;

#define ES_VSQUARE(com) ((struct es_vsquare *)(com))

__BEGIN_DECLS
extern ES_ComponentClass esVSquareClass;
__END_DECLS
