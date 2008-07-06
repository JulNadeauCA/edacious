/* Public domain */

/* Utility functions for stamping commonly used linear components */

#include <eda.h>

#define N SIM(dc)->ckt->n

#define AddG(k,l,val)	dc->A->v[k][l]	   += val
#define AddB(k,l,val)	dc->A->v[k][N+l]   += val
#define AddC(k,l,val)	dc->A->v[N+k][l]   += val
#define AddD(k,l,val)	dc->A->v[N+k][N+l] += val
#define AddI(k,val)	dc->z->v[k][0]	   += val
#define AddV(k,val)	dc->z->v[N+k][0]   += val

#define SetG(k,l,val)	dc->A->v[k][l]	   = val
#define SetB(k,l,val)	dc->A->v[k][N+l]   = val
#define SetC(k,l,val)	dc->A->v[N+k][l]   = val
#define SetD(k,l,val)	dc->A->v[N+k][N+l] = val
#define SetI(k,val)	dc->z->v[k][0]	   = val
#define SetV(k,val)	dc->z->v[N+k][0]   = val

void
StampConductance(M_Real g, Uint k, Uint l, ES_SimDC *dc)
{
        if (k != 0) {
		AddG(k, k, g);
        }
        if (l != 0) {
                AddG(l, l, g);
        }
        if (k != 0 && l != 0) {
		AddG(k, l, -g);
		AddG(l, k, -g);
        }
}

void
StampCurrentSource(M_Real I, Uint k, Uint l, ES_SimDC *dc)
{
	if (k != 0){
		AddI(k, I);
	}
	if (l != 0){
		AddI(l, -I);
	}
}

void
StampVCCS(M_Real g, Uint k, Uint l, Uint p, Uint q, ES_SimDC *dc)
{
	if (p != 0){
		if (k != 0){
			AddG(p, k, g);
		}
		if (l != 0){
			AddG(p, l, -g);
		}
	}
	if (q != 0){
		if (k != 0){
			AddG(q, k, -g);
		}
		if (l != 0){
			AddG(q, l, g);
		}
	}
}

void 
StampVoltageSource(M_Real voltage, Uint k, Uint l, Uint v, ES_SimDC *dc)
{
	if (k != 0) {
		SetB(k, v, 1.0);
		SetC(v, k, 1.0);
	}
	if (l != 0) {
		SetB(l, v, -1.0);
		SetC(v, l, -1.0);
	}

	SetV(v, voltage);
}

void
StampThevenin(M_Real voltage, Uint k, Uint l, Uint v, M_Real resistance, ES_SimDC *dc)
{
	
	StampVoltageSource(voltage, k, l, v, dc);
	SetD(v, v, -resistance);
}
