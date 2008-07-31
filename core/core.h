/*	Public domain	*/

#ifndef _EDACIOUS_CORE_H_
#define _EDACIOUS_CORE_H_

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>
#include <freesg/m/m.h>
#include <freesg/m/m_gui.h>

#include <edacious/core/core_begin.h>

#include <edacious/core/schem.h>
#include <edacious/core/component.h>
#include <edacious/core/sim.h>
#include <edacious/core/integration.h>
#include <edacious/core/dc.h>
#include <edacious/core/circuit.h>
#include <edacious/core/icons.h>
#include <edacious/core/scope.h>
#include <edacious/core/stamp.h>
#include <edacious/core/spice.h>
#include <edacious/core/wire.h>

#ifdef _ES_INTERNAL
# include <edacious/core/core_internal.h>
#endif

__BEGIN_DECLS
extern AG_Object   esVfsRoot;		 /* General-purpose VFS */
extern void       *esCoreClasses[];	 /* Base object classes */
extern const void *esSchematicClasses[]; /* Base schematic VG classes */
extern const char *esEditableClasses[];	 /* User-editable object classes */
extern void      **esComponentClasses;	 /* Component model classes */
extern Uint        esComponentClassCount;

void	ES_CoreInit(void);
void	ES_CoreDestroy(void);
void	ES_RegisterClass(void *);
void	ES_UnregisterClass(void *);
void	ES_SetObjectOpenHandler(AG_Window *(*fn)(void *));
void	ES_SetObjectCloseHandler(void (*fn)(void *));
void	ES_InsertComponent(ES_Circuit *, VG_Tool *, ES_ComponentClass *);

AG_Window *ES_OpenObject(void *);
void       ES_CloseObject(void *);
__END_DECLS

#include <edacious/core/core_close.h>
#endif /* _EDACIOUS_CORE_H_ */
