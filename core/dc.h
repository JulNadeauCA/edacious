/*	Public domain	*/

/* Available integration methods */
enum es_integration_method {
	BE,                     /* Backwards Euler */
	FE,                     /* Forward Euler */
	TR                      /* Trapezoidal rule */
};

typedef struct es_sim_dc {
	struct es_sim _inherit;

	enum es_integration_method method;	/* Method of integration used */
	
	AG_Timeout toUpdate;	/* Timer for simulation updates */
	M_Real Telapsed;        /* Simulated elapsed time (s) */
	M_Real deltaT;          /* Simulated time since last iteration (s) */
	Uint   ticksDelay;	/* Simulation speed (delay ms) */
	Uint32 ticksLastStep;	/* Processing time of last step (ticks) */
	
	Uint isDamped;		/* 1 if any components had to damp voltage
	                           guesses in the previous iteration, 0
				   otherwise */
	Uint inputStep;		/* flag that can be set by active components
	                           to aid simulation: 1 if there is a
				   discontinuity in input, 0 otherwise */
	Uint retriesMax;	/* Maximum number of times time step may be
				   decimated before halting simulation */

	Uint itersMax;		/* Limit on iterations/step */
	Uint itersHigh;		/* Most iterations/step recorded */
	Uint itersLow;		/* Least iterations/step recorded */
	M_Real stepLow;		/* Smallest timestep used */
	M_Real stepHigh;	/* Largest timestep used */

	M_Real T0;		/* Reference temperature */
	M_Matrix *A;		/* Block matrix [G,B; C,D] */
	M_Vector *z;		/* Right-hand side vector (i,e) */
	M_Vector *x;		/* Vector of unknowns (v,j) */
	
	M_Vector *xPrevIter;	/* Solution from last iteration */
	M_Vector **xPrevSteps;	/* Solutions from last steps */
	int stepsToKeep;        /* Number of solutions to keep in xPrevSteps */

	M_Real *groundNode;     /* Pointer to A(0, 0) */
} ES_SimDC;

__BEGIN_DECLS
const ES_SimOps esSimDcOps;
__END_DECLS
