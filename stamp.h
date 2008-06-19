/* Public domain */

/* Utility functions for stamping commonly used linear components */

__BEGIN_DECLS

void StampConductance(M_Real g, Uint k, Uint l, M_Matrix *G);
void StampCurrentSource(M_Real I, Uint k, Uint l, M_Vector *i);
void StampVCCS(M_Real g, Uint k, Uint l, Uint p, Uint q, M_Matrix *G);
void StampVoltageSource(M_Real voltage, Uint k, Uint j, Uint v, M_Matrix *B, M_Matrix *C, M_Vector *e);

__END_DECLS
