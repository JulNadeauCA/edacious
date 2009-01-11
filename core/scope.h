/*	Public domain	*/

struct m_plotter;

typedef struct es_scope {
	struct ag_object obj;
	struct m_plotter *plotter;
	ES_Circuit *ckt;
} ES_Scope;

__BEGIN_DECLS
extern AG_ObjectClass esScopeClass;
ES_Scope *ES_ScopeNew(ES_Circuit *, const char *);
__END_DECLS
