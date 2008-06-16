/* Public domain */

/* Utility functions for stamping commonly used linear components */

#include <eda.h>

void
StampConductance(M_Real g, uint k, uint l, M_Matrix *G)
{
        if (k != 0) {
                G->v[k][k] += g;
        }
        if (l != 0) {
                G->v[l][l] += g;
        }
        if (k != 0 && l != 0) {
                G->v[k][l] -= g;
                G->v[l][k] -= g;
        }
}

void
StampCurrentSource(M_Real I, uint k, uint l, M_Vector *i)
{
	if (k != 0){
		i->v[k][0] = I;
	}
	if (l != 0){
		i->v[l][0] = -I;
	}
}

void
StampVCCS(M_Real g, uint k, uint l, uint p, uint q, M_Matrix *G)
{
	if (p != 0){
		if (k != 0){
			G->v[p][k] += g;
		}
		if (l != 0){
			G->v[p][l] -= g;
		}
	}
	if (q != 0){
		if (k != 0){
			G->v[q][k] -= g;
		}
		if (l != 0){
			G->v[q][l] += g;
		}
	}
}
