/*	Public domain	*/

struct m_plotter;

/* Oscilloscope tool */
typedef struct es_scope {
	struct ag_object obj;			/* AG_Object -> ES_Scope */
	struct m_plotter *plotter;
	ES_Circuit *ckt;
} ES_Scope;

#define ESSCOPE(p)             ((ES_Scope *)(p))
#define ESCSCOPE(obj)          ((const ES_Scope *)(obj))
#define ES_SCOPE_SELF()          ESSCOPE( AG_OBJECT(0,"ES_Scope:*") )
#define ES_SCOPE_PTR(n)          ESSCOPE( AG_OBJECT((n),"ES_Scope:*") )
#define ES_SCOPE_NAMED(n)        ESSCOPE( AG_OBJECT_NAMED((n),"ES_Scope:*") )
#define ES_CONST_SCOPE_SELF()   ESCSCOPE( AG_CONST_OBJECT(0,"ES_Scope:*") )
#define ES_CONST_SCOPE_PTR(n)   ESCSCOPE( AG_CONST_OBJECT((n),"ES_Scope:*") )
#define ES_CONST_SCOPE_NAMED(n) ESCSCOPE( AG_CONST_OBJECT_NAMED((n),"ES_Scope:*") )

__BEGIN_DECLS
extern AG_ObjectClass esScopeClass;
ES_Scope *ES_ScopeNew(ES_Circuit *, const char *);
__END_DECLS
