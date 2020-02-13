/*	Public domain	*/

struct es_circuit;
struct ag_console;

typedef struct es_sim_ops {
	char *name;
	AG_StaticIcon *icon;
	size_t size;
	void (*init)(void *);
	void (*destroy)(void *);
	void (*start)(void *);
	void (*stop)(void *);
	void (*cktmod)(void *, struct es_circuit *);
	M_Real (*node_voltage)(void *, int);
	M_Real (*node_voltage_prev_step)(void *, int, int);
	M_Real (*branch_current)(void *, int);
	M_Real (*branch_current_prev_step)(void *, int, int);
	AG_Window *(*edit)(void *, struct es_circuit *);
} ES_SimOps;

typedef struct es_sim {
	struct es_circuit *ckt;		/* Circuit being analyzed */
	const ES_SimOps *ops;		/* Generic operations */
	AG_Window *win;			/* Settings window */
	int running;			/* Continous simulation in process */
} ES_Sim;

#define ESSIM(p) ((ES_Sim *)(p))

#ifdef _ES_INTERNAL
#define SIM(p) ((ES_Sim *)(p))
#endif

__BEGIN_DECLS
extern const ES_SimOps *esSimOps[];

void ES_SimInit(void *, const ES_SimOps *);
void ES_SimDestroy(void *);
void ES_SimEdit(AG_Event *);
void ES_SimLog(void *, const char *, ...);
__END_DECLS
