/*	Public domain	*/

typedef struct es_sim_dc {
	struct es_sim _inherit;

	enum es_integration_method method;	/* Method of integration used */
	
	AG_Timeout toUpdate;	/* Timer for simulation updates */
	M_Real Telapsed;        /* Simulated elapsed time (s) */
	M_Real deltaT;          /* Simulated time since last iteration (s) */
	Uint   ticksDelay;	/* Simulation speed (delay ms) */
	Uint currStep;          /* Number of current step */
	
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

	int stepsToKeep;        /* Number of previous solutions to keep */
	M_Vector **xPrevSteps;	/* Solutions from last steps */
	M_Real *deltaTPrevSteps;/* Previous timesteps. deltaTPrevSteps[i] is
				 * the timestep used to compute xPrevSteps[i] */

	M_Real *groundNode;     /* Pointer to A(0, 0) */
} ES_SimDC;

__BEGIN_DECLS
const ES_SimOps esSimDcOps;
__END_DECLS
