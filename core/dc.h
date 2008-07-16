/*	Public domain	*/

/* Available integration methods */
typedef enum integrationMethod {
	BE,                     /* Backwards Euler */
	FE,                     /* Forward Euler */
	TR                      /* Trapezoidal rule */
} IntegrationMethod;

static const char *IntegrationMethodStr[];

typedef struct es_sim_dc {
	struct es_sim _inherit;

	IntegrationMethod method;/*Method of integration used */
	
	AG_Timeout toUpdate;	/* Timer for simulation updates */
	M_Real Telapsed;        /* Simulated elapsed time (s) */
	M_Real deltaT;          /* Simulated time since last iteration (s) */
	int maxSpeed;           /* Maximum number of iterations/sec */
	Uint32 timeLastStep;    /* Time (as measured in SDL ticks) of last step */
	
	Uint isDamped;		/* 1 if any components had to damp voltage guesses in the previous iteration, 0 otherwise */
	Uint inputStep;		/* flag that can be set by active components to aid simulation: 1 if there is a discontinuity in input, 0 otherwise */
	Uint firstIter;		/* 1 if the first NR iteration for a given time step is in progress, 0 otherwise */

	Uint itersMax;		/* Maximum solver iterations per step */
	Uint itersHiwat;	/* Most iterations/step in last simulation */
	Uint itersLowat;	/* Least iterations/step in last simulation */

	Uint retriesMax;	/* Maximum number of times time step may be decimated before halting simulation */
	
	M_Real T0;		/* Reference temperature */

	M_Matrix *A;		/* Block matrix [G,B; C,D] */

	M_Vector *z;		/* Right-hand side vector (i,e) */

	M_Vector *x;		/* Vector of unknowns (v,j) */
	M_Vector *xPrevIter;	/* Vector of unknowns from last iteration */
	M_Vector *xPrevStep;	/* Vector of unknowns from last timestep */

} ES_SimDC;

__BEGIN_DECLS
const ES_SimOps esSimDcOps;
__END_DECLS
