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
