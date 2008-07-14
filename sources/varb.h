/*	Public domain	*/

typedef struct es_varb {
	struct es_vsource _inherit;
	Uint flags;
#define ES_VARB_ERROR	0x01		/* Parser failed */
	char exp[64];			/* Expression to interpret */
} ES_VArb;

#define ES_VARB(com) ((struct es_varb *)(com))

__BEGIN_DECLS
extern ES_ComponentClass esVArbClass;
__END_DECLS
