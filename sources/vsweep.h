/*	Public domain	*/

typedef struct es_vsweep {
	struct es_vsource _inherit;
	M_Real v1;			/* Start voltage */
	M_Real v2;			/* End voltage */
	M_Real t;			/* Duration */
	int count;			/* Repeat count (0 = loop) */
} ES_VSweep;

#define ES_VSWEEP(com) ((struct es_vsweep *)(com))

__BEGIN_DECLS
extern ES_ComponentClass esVSweepClass;
__END_DECLS
