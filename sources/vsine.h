/*	Public domain	*/

typedef struct es_vsine {
	struct es_vsource _inherit;
	M_Real vPeak;			/* Peak voltage */
	M_Real f;			/* Frequency (Hz) */
} ES_VSine;

#define ES_VSINE(com) ((struct es_vsine *)(com))

__BEGIN_DECLS
extern ES_ComponentClass esVSineClass;
__END_DECLS
