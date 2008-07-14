/*	Public domain	*/

#ifndef _EDACIOUS_EDACIOUS_H_
#define _EDACIOUS_EDACIOUS_H_

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>
#include <freesg/m/m.h>
#include <freesg/m/m_gui.h>

#include <edacious/eda_begin.h>

#include <edacious/core/schem.h>
#include <edacious/core/component.h>
#include <edacious/core/circuit.h>
#include <edacious/core/icons.h>
#include <edacious/core/stamp.h>
#include <edacious/core/spice.h>
#include <edacious/core/wire.h>

#include <edacious/sources/isource.h>
#include <edacious/sources/vsource.h>
#include <edacious/sources/varb.h>
#include <edacious/sources/vsine.h>
#include <edacious/sources/vsquare.h>
#include <edacious/sources/vsweep.h>

#include <edacious/macro/and_gate.h>
#include <edacious/macro/digital.h>
#include <edacious/macro/inverter.h>
#include <edacious/macro/or_gate.h>

#include <edacious/measurement/scope.h>
#include <edacious/measurement/logic_probe.h>

#include <edacious/generic/capacitor.h>
#include <edacious/generic/diode.h>
#include <edacious/generic/ground.h>
#include <edacious/generic/inductor.h>
#include <edacious/generic/led.h>
#include <edacious/generic/nmos.h>
#include <edacious/generic/npn.h>
#include <edacious/generic/pmos.h>
#include <edacious/generic/pnp.h>
#include <edacious/generic/resistor.h>
#include <edacious/generic/semiresistor.h>
#include <edacious/generic/spst.h>
#include <edacious/generic/spdt.h>

#ifdef _ES_INTERNAL
# include <edacious/eda_internal.h>
#endif

__BEGIN_DECLS
extern AG_Object   esVfsRoot;		 /* General-purpose VFS */
extern const void *esSchematicClasses[]; /* Schematic VG classes */
extern void       *esBaseClasses[];	 /* Base object classes */
extern void       *esComponentClasses[]; /* Component model classes */
extern const char *esEditableClasses[];	 /* Editable classes */

void       ES_InitSubsystem(void);
AG_Window *ES_CreateEditionWindow(void *);
void       ES_CloseEditionWindow(void *);
__END_DECLS

#include <edacious/eda_close.h>
#endif /* _EDACIOUS_EDACIOUS_H_ */
