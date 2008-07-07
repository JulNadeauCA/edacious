/* Public domain */

/* Utility functions for stamping commonly used linear components */

__BEGIN_DECLS

/* Each element is composed of three symbols : its stamp size, an init function,
 * and a stamp function. The caller component must reserve a memory block of stamp size
 * M_Real elements, and pass it to the init function for it to write the adress
 * where subsequent writes will be done. Then the component passes the StampData
 * and the values to write each time it wants to stamp */

typedef M_Real *StampData[];

/* Conductance */
#define STAMP_CONDUCTANCE_SIZE 4
void InitStampConductance(Uint k, Uint l, StampData s, ES_SimDC *dc);
void StampConductance(M_Real g, StampData s);

/* Current source */
#define STAMP_CURRENT_SOURCE_SIZE 2
void InitStampCurrentSource(Uint k, Uint l, StampData s, ES_SimDC *dc);
void StampCurrentSource(M_Real I, StampData s);

/* Voltage controlled current source */
#define STAMP_VCCS_SIZE 4
void InitStampVCCS(Uint k, Uint l, Uint p, Uint q, StampData s, ES_SimDC *dc);
void StampVCCS(M_Real g, StampData s);

/* Independant voltage source */
#define STAMP_VOLTAGE_SOURCE_SIZE 5
void InitStampVoltageSource(Uint k, Uint l, Uint v, StampData s, ES_SimDC *dc);
void StampVoltageSource(M_Real voltage, StampData s);

/* Thevenin model */
#define STAMP_THEVENIN_SIZE 6
void InitStampThevenin(Uint k, Uint l, Uint v, StampData s, ES_SimDC *dc);
void StampThevenin(M_Real voltage, M_Real resistance, StampData s);

__END_DECLS
