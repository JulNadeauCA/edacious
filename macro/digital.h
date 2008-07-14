/*	Public domain	*/

typedef enum es_digital_class {
	ES_DIGITAL_GATE,
	ES_DIGITAL_BUFFERS,
	ES_DIGITAL_FLIPFLOPS,
	ES_DIGITAL_MSI,
	ES_DIGITAL_LSI
} ES_DigitalClass;

typedef enum es_digital_state {
	ES_LOW,			/* Logical LOW */
	ES_HIGH,		/* Logical HIGH */
	ES_HI_Z,		/* High impedance output */
	ES_INVALID		/* Invalid logic input */
} ES_LogicState;

typedef struct es_digital {
	struct es_component _inherit;
	int VccPort;		/* DC supply port */
	int GndPort;		/* Ground port */
	M_Vector *G;		/* Current conductive state */
	M_Range Vcc;		/* DC supply voltage (operating) */
	M_Range Tamb;		/* Operating ambient temperature */
	M_Range Idd;		/* Quiescent current */
	M_Range Vol, Voh;	/* Output voltage LOW/HIGH (buffered) */
	M_Range Vil, Vih;	/* Input voltage LOW/HIGH (buffered) */
	M_Range Iol;		/* Output (sink current); LOW */
	M_Range Ioh;		/* Output (source current); HIGH */
	M_Range Iin;		/* Input leakage current (+-) */
	M_Range Iozh;		/* 3-state output leakage current; HIGH */
	M_Range Iozl;		/* 3-state output leakage current; LOW */
	M_TimeRange Tthl;	/* Output transition time; HIGH->LOW */
	M_TimeRange Ttlh;	/* Output transition time; LOW->HIGH */
} ES_Digital;

#define ES_DIGITAL(p) ((ES_Digital *)(p))

#ifdef _ES_INTERNAL
# define DIGITAL(p) ES_DIGITAL(p)
# define PAIR_MATCHES(pair,a,b) \
	((pair->p1->n == (a) && pair->p2->n == (b)) || \
	 (pair->p2->n == (a) && pair->p1->n == (b)))
#endif

__BEGIN_DECLS
extern ES_ComponentClass esDigitalClass;

void	 ES_DigitalInitPorts(void *, const ES_Port *);
void	 ES_DigitalStepIter(void *, ES_SimDC *);
void	 ES_DigitalSetVccPort(void *, int);
void	 ES_DigitalSetGndPort(void *, int);
int	 ES_LogicOutput(void *, const char *, ES_LogicState);
int	 ES_LogicInput(void *, const char *);
__END_DECLS
