/* Public domain */

/* Utility functions for stamping commonly used linear components */

__BEGIN_DECLS

void StampConductance(M_Real g, Uint k, Uint l, ES_SimDC *dc);

void StampCurrentSource(M_Real I, Uint k, Uint l, ES_SimDC *dc);

void StampVCCS(M_Real g, Uint k, Uint l, Uint p, Uint q, ES_SimDC *dc);

void StampVoltageSource(M_Real voltage, Uint k, Uint l, Uint v, ES_SimDC *dc);

void StampThevenin(M_Real voltage, Uint k, Uint l, Uint v, M_Real resistance, ES_SimDC *dc);

__END_DECLS
