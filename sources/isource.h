/*	Public domain	*/

typedef struct es_isource {
	struct es_component _inherit;
	M_Real I;				/* Current (amps) */
	
	StampCurrentSourceData s;
} ES_Isource;

__BEGIN_DECLS
extern ES_ComponentClass esIsourceClass;
__END_DECLS
