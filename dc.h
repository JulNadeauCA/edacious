/*	Public domain	*/

typedef struct es_sim_dc {
	struct es_sim _inherit;

	AG_Timeout toUpdate;	/* Timer for simulation updates */
	Uint32 Telapsed;        /* Simulated elapsed time (ns) */
	int maxSpeed;           /* Maximum number of iterations/sec */
	Uint32 timeLastStep;    /* time (as measured in SDL ticks) of last step */
	
	Uint itersMax;		/* Maximum solver iterations per step */
	Uint itersHiwat;	/* Most iterations/step in last simulation */
	Uint itersLowat;	/* Least iterations/step in last simulation */

	M_Real T0;		/* Reference temperature */

	M_Matrix *A;		/* Block matrix [G,B; C,D] */
	M_Matrix *G;		/* Reduced conductance matrix */
	M_Matrix *B;		/* Voltage source matrix */
	M_Matrix *C;		/* Transpose of B */
	M_Matrix *D;		/* Dependent source matrix */
	M_Matrix *LU;		/* LU factorizations of A */

	M_Vector *z;		/* Right-hand side vector (i,e) */
	M_Vector *i;		/* Independent current sources */
	M_Vector *e;		/* Independent voltage sources */

	M_Vector *x;		/* Vector of unknowns (v,j) */
	M_Vector *v;		/* Voltages */
	M_Vector *j;		/* Currents through independent vsources */
	
	M_IntVector *piv;	/* Pivot information from last factorization */
} ES_SimDC;

__BEGIN_DECLS
const ES_SimOps esSimDcOps;
__END_DECLS
