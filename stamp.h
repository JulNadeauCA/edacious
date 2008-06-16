/* Public domain */

/* Utility functions for stamping commonly used linear components */

__BEGIN_DECLS

void StampConductance(M_Real g, uint k, uint l, M_Matrix *G);
void StampCurrentSource(M_Real I, uint k, uint l, M_Vector *i);
void StampVCCS(M_Real g, uint k, uint l, uint p, uint q, M_Matrix *G);

__END_DECLS
