/*	Public domain	*/

typedef struct es_spst {
	struct es_component _inherit;	/* ES_Component -> ES_Spst */
	M_Real Ron;			/* ON-resistance */
	M_Real Roff;			/* OFF-resistance */
	M_Real g;			/* Conductance at the beginning of the last time step */
	int state;			/* Last state */
	StampConductanceData s;
} ES_Spst;

#define ESSPST(p)             ((ES_Spst *)(p))
#define ESCSPST(obj)          ((const ES_Spst *)(obj))
#define ES_SPST_SELF()          ESSPST( AG_OBJECT(0,"ES_Circuit:ES_Component:ES_Spst:*") )
#define ES_SPST_PTR(n)          ESSPST( AG_OBJECT((n),"ES_Circuit:ES_Component:ES_Spst:*") )
#define ES_SPST_NAMED(n)        ESSPST( AG_OBJECT_NAMED((n),"ES_Circuit:ES_Component:ES_Spst:*") )
#define ES_CONST_SPST_SELF()   ESCSPST( AG_CONST_OBJECT(0,"ES_Circuit:ES_Component:ES_Spst:*") )
#define ES_CONST_SPST_PTR(n)   ESCSPST( AG_CONST_OBJECT((n),"ES_Circuit:ES_Component:ES_Spst:*") )
#define ES_CONST_SPST_NAMED(n) ESCSPST( AG_CONST_OBJECT_NAMED((n),"ES_Circuit:ES_Component:ES_Spst:*") )

__BEGIN_DECLS
extern ES_ComponentClass esSpstClass;
__END_DECLS
