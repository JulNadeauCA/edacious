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
	M_Real Vcc;		/* DC supply voltage (operating) */
	M_Real VoL, VoH;	/* Output voltage (buffered) */
	M_Real ViH_min; 	/* Min input voltage for HIGH (buffered) */
	M_Real ViL_max;		/* Max input voltage for LOW (buffered) */
	M_Real Idd;		/* Quiescent current */
	M_Real IoL, IoH;	/* Output (sink current) */
	M_Real Iin;		/* Input leakage current (+-) */
	M_Real IozH, IozL;	/* 3-state output leakage current */
	M_Time tHL, tLH;		/* Output transition time; HIGH<->LOW */
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
