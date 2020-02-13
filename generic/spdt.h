/*	Public domain	*/

typedef struct es_spdt {
	struct es_component _inherit;	/* ES_Component -> ES_Spdt */
	M_Real Ron;			/* ON-resistance */
	M_Real Roff;			/* OFF-resistance */
	int state;			/* Last state */
} ES_Spdt;

#define ESSPDT(p)             ((ES_Spdt *)(p))
#define ESCSPDT(obj)          ((const ES_Spdt *)(obj))
#define ES_SPDT_SELF()          ESSPDT( AG_OBJECT(0,"ES_Circuit:ES_Component:ES_Spdt:*") )
#define ES_SPDT_PTR(n)          ESSPDT( AG_OBJECT((n),"ES_Circuit:ES_Component:ES_Spdt:*") )
#define ES_SPDT_NAMED(n)        ESSPDT( AG_OBJECT_NAMED((n),"ES_Circuit:ES_Component:ES_Spdt:*") )
#define ES_CONST_SPDT_SELF()   ESCSPDT( AG_CONST_OBJECT(0,"ES_Circuit:ES_Component:ES_Spdt:*") )
#define ES_CONST_SPDT_PTR(n)   ESCSPDT( AG_CONST_OBJECT((n),"ES_Circuit:ES_Component:ES_Spdt:*") )
#define ES_CONST_SPDT_NAMED(n) ESCSPDT( AG_CONST_OBJECT_NAMED((n),"ES_Circuit:ES_Component:ES_Spdt:*") )

__BEGIN_DECLS
extern ES_ComponentClass esSpdtClass;
__END_DECLS
