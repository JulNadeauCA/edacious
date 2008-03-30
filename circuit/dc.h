/*	Public domain	*/

typedef struct es_dc_analysis {
	ES_Sim sim;

	Uint32 Telapsed;	/* Simulated time elapsed (ns) */
	Uint32 TavgReal;	/* Average step real time */
	int speed;		/* Simulation speed (updates/sec) */
	AG_Timeout update_to;	/* Timer for simulation updates */
	Uint max_iters;		/* Maximum solver iterations per step */
	Uint iters_hiwat;
	Uint iters_lowat;

	SC_Matrix *A;		/* Block matrix [G,B; C,D] */
	SC_Matrix *G;		/* Reduced conductance matrix */
	SC_Matrix *B;		/* Voltage source matrix */
	SC_Matrix *C;		/* Transpose of B */
	SC_Matrix *D;		/* Dependent source matrix */
	SC_Matrix *LU;		/* LU factorizations of A */

	SC_Vector *z;		/* Right-hand side vector (i,e) */
	SC_Vector *i;		/* Independent current sources */
	SC_Vector *e;		/* Independent voltage sources */

	SC_Vector *x;		/* Vector of unknowns (v,j) */
	SC_Vector *v;		/* Voltages */
	SC_Vector *j;		/* Currents through independent vsources */
	
	SC_Ivector *piv;	/* Pivot information from last factorization */
} ES_SimDC;

__BEGIN_DECLS
const ES_SimOps esSimDcOps;
__END_DECLS
