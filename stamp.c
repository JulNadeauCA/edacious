/* Public domain */

/* Utility functions for stamping commonly used linear components */

#include <eda.h>

void
StampConductance(M_Real g, Uint k, Uint l, M_Matrix *G)
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
StampCurrentSource(M_Real I, Uint k, Uint l, M_Vector *i)
{
	if (k != 0){
		i->v[k][0] += I;
	}
	if (l != 0){
		i->v[l][0] -= I;
	}
}

void
StampVCCS(M_Real g, Uint k, Uint l, Uint p, Uint q, M_Matrix *G)
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

void 
StampVoltageSource(M_Real voltage, Uint k, Uint l, Uint v, M_Matrix *B, M_Matrix *C, M_Vector *e)
{
	if (k != 0) {
		B->v[k][v] = 1.0;
		C->v[v][k] = 1.0;
	}
	if (l != 0) {
		B->v[l][v] = -1.0;
		C->v[v][l] = -1.0;
	}

	e->v[v][0] = voltage;
}

void
StampThevenin(M_Real voltage, Uint k, Uint l, Uint v, M_Matrix *B, M_Matrix *C, M_Vector *e, M_Real resistance, M_Matrix *D)
{
	StampVoltageSource(voltage, k, l, v, B, C, e);
	D->v[v][v] = - resistance;
}
