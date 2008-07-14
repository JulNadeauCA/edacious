/*	Public domain	*/

typedef struct es_logic_probe {
	struct es_component _inherit;

	M_Real Vhigh;		/* Minimum HIGH voltage */
	int state;		/* Last state */
} ES_LogicProbe;

__BEGIN_DECLS
extern ES_ComponentClass esLogicProbeClass;
__END_DECLS
