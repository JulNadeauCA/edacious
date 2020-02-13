/*	Public domain	*/

typedef struct es_capacitor {
	struct es_component _inherit;	/* ES_Component -> ES_Capacitor */
	int vIdx;			/* Index into e */
	M_Real C;			/* Capacitance */
	M_Real V0;			/* Starting voltage */
	M_Real r, v;			/* Parameters of Thevenin model */
	StampTheveninData s;
} ES_Capacitor;

#define ESCAPACITOR(p)             ((ES_Capacitor *)(p))
#define ESCCAPACITOR(obj)          ((const ES_Capacitor *)(obj))
#define ES_CAPACITOR_SELF()          ESCAPACITOR( AG_OBJECT(0,"ES_Circuit:ES_Component:ES_Capacitor:*") )
#define ES_CAPACITOR_PTR(n)          ESCAPACITOR( AG_OBJECT((n),"ES_Circuit:ES_Component:ES_Capacitor:*") )
#define ES_CAPACITOR_NAMED(n)        ESCAPACITOR( AG_OBJECT_NAMED((n),"ES_Circuit:ES_Component:ES_Capacitor:*") )
#define ES_CONST_CAPACITOR_SELF()   ESCCAPACITOR( AG_CONST_OBJECT(0,"ES_Circuit:ES_Component:ES_Capacitor:*") )
#define ES_CONST_CAPACITOR_PTR(n)   ESCCAPACITOR( AG_CONST_OBJECT((n),"ES_Circuit:ES_Component:ES_Capacitor:*") )
#define ES_CONST_CAPACITOR_NAMED(n) ESCCAPACITOR( AG_CONST_OBJECT_NAMED((n),"ES_Circuit:ES_Component:ES_Capacitor:*") )

__BEGIN_DECLS
extern ES_ComponentClass esCapacitorClass;
__END_DECLS
