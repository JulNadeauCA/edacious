/*	Public domain	*/

typedef enum es_digital_class {
	ES_DIGITAL_GATE,
	ES_DIGITAL_BUFFERS,
	ES_DIGITAL_FLIPFLOPS,
	ES_DIGITAL_MSI,
	ES_DIGITAL_LSI
} ES_DigitalClass;

typedef enum es_digital_state {
	ES_LOW,				/* Logical LOW */
	ES_HIGH,			/* Logical HIGH */
	ES_HI_Z,			/* High impedance output */
	ES_INVALID			/* Invalid logic input */
} ES_LogicState;

typedef struct es_digital {
	struct es_component _inherit;	/* ES_Component -> ES_Digital */
	int VccPort;			/* DC supply port */
	int GndPort;			/* Ground port */
	M_Vector *G;			/* Current conductive state */
	M_Real Vcc;			/* DC supply voltage (operating) */
	M_Real VoL, VoH;		/* Output voltage (buffered) */
	M_Real ViH_min; 		/* Min input voltage for HIGH (buffered) */
	M_Real ViL_max;			/* Max input voltage for LOW (buffered) */
	M_Real Idd;			/* Quiescent current */
	M_Real IoL, IoH;		/* Output (sink current) */
	M_Real Iin;			/* Input leakage current (+-) */
	M_Real IozH, IozL;		/* 3-state output leakage current */
	M_Time tHL, tLH;		/* Output transition time; HIGH<->LOW */
} ES_Digital;

#define ESDIGITAL(p)             ((ES_Digital *)(p))
#define ESCDIGITAL(obj)          ((const ES_Digital *)(obj))
#define ES_DIGITAL_SELF()          ESDIGITAL( AG_OBJECT(0,"ES_Circuit:ES_Component:ES_Digital:*") )
#define ES_DIGITAL_PTR(n)          ESDIGITAL( AG_OBJECT((n),"ES_Circuit:ES_Component:ES_Digital:*") )
#define ES_DIGITAL_NAMED(n)        ESDIGITAL( AG_OBJECT_NAMED((n),"ES_Circuit:ES_Component:ES_Digital:*") )
#define ES_CONST_DIGITAL_SELF()   ESCDIGITAL( AG_CONST_OBJECT(0,"ES_Circuit:ES_Component:ES_Digital:*") )
#define ES_CONST_DIGITAL_PTR(n)   ESCDIGITAL( AG_CONST_OBJECT((n),"ES_Circuit:ES_Component:ES_Digital:*") )
#define ES_CONST_DIGITAL_NAMED(n) ESCDIGITAL( AG_CONST_OBJECT_NAMED((n),"ES_Circuit:ES_Component:ES_Digital:*") )

__BEGIN_DECLS
extern ES_ComponentClass esDigitalClass;

void	 ES_DigitalInitPorts(void *, const ES_Port *);
void	 ES_DigitalStepIter(void *, ES_SimDC *);
void	 ES_DigitalSetVccPort(void *, int);
void	 ES_DigitalSetGndPort(void *, int);
int	 ES_LogicOutput(void *, const char *, ES_LogicState);
int	 ES_LogicInput(void *, const char *);
__END_DECLS
