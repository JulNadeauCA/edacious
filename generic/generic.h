/*	Public domain	*/

#include <edacious/core/core_begin.h>

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

__BEGIN_DECLS
extern void *esGenericClasses[];
void ES_GenericInit(void);
void ES_GenericDestroy(void);
__END_DECLS

#include <edacious/core/core_close.h>
