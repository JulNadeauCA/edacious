/*	Public domain	*/

typedef struct es_vsource {
	struct es_component _inherit;		/* ES_Component -> ES_Vsource */
	int vIdx;				/* Index into e */
	M_Real v;				/* Effective voltage */
	StampVoltageSourceData s;
	TAILQ_HEAD(,es_loop) loops;		/* Forward loops */
	Uint nLoops;
	ES_Port	**lstack;			/* For loop computation */
	Uint nlstack;
} ES_Vsource;

#define ESVSOURCE(p)             ((ES_Vsource *)(p))
#define ESCVSOURCE(obj)          ((const ES_Vsource *)(obj))
#define ES_VSOURCE_SELF()          ESVSOURCE( AG_OBJECT(0,"ES_Circuit:ES_Component:ES_Vsource:*") )
#define ES_VSOURCE_PTR(n)          ESVSOURCE( AG_OBJECT((n),"ES_Circuit:ES_Component:ES_Vsource:*") )
#define ES_VSOURCE_NAMED(n)        ESVSOURCE( AG_OBJECT_NAMED((n),"ES_Circuit:ES_Component:ES_Vsource:*") )
#define ES_CONST_VSOURCE_SELF()   ESCVSOURCE( AG_CONST_OBJECT(0,"ES_Circuit:ES_Component:ES_Vsource:*") )
#define ES_CONST_VSOURCE_PTR(n)   ESCVSOURCE( AG_CONST_OBJECT((n),"ES_Circuit:ES_Component:ES_Vsource:*") )
#define ES_CONST_VSOURCE_NAMED(n) ESCVSOURCE( AG_CONST_OBJECT_NAMED((n),"ES_Circuit:ES_Component:ES_Vsource:*") )

__BEGIN_DECLS
extern ES_ComponentClass esVsourceClass;

void	 ES_VsourceFindLoops(ES_Vsource *);
void	 ES_VsourceFreeLoops(ES_Vsource *);
double	 ES_VsourceVoltage(void *, int, int);
__END_DECLS
