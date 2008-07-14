/*	Public domain	*/

typedef struct es_vsource {
	struct es_component _inherit;

	int vIdx;				/* Index into e */
	M_Real v;				/* Effective voltage */
	M_Real *s[STAMP_VOLTAGE_SOURCE_SIZE];	/* Matrix stamp */

	TAILQ_HEAD(,es_loop) loops;		/* Forward loops */
	Uint nloops;
	ES_Port	**lstack;			/* For loop computation */
	Uint nlstack;
} ES_Vsource;

#define ES_VSOURCE(com) ((struct es_vsource *)(com))
#ifdef _ES_INTERNAL
# define VSOURCE(com) ES_VSOURCE(com)
#endif

__BEGIN_DECLS
extern ES_ComponentClass esVsourceClass;

void	 ES_VsourceFindLoops(ES_Vsource *);
void	 ES_VsourceFreeLoops(ES_Vsource *);
double	 ES_VsourceVoltage(void *, int, int);
__END_DECLS
