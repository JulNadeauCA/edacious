/*	Public domain	*/

#ifndef _EDACIOUS_CORE_H_
#define _EDACIOUS_CORE_H_

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>
#include <agar/math/m.h>
#include <agar/math/m_gui.h>

#include <edacious/config/edacious_version.h>

#include <edacious/core/core_begin.h>

#include <edacious/core/schem.h>
#include <edacious/core/sim.h>
#include <edacious/core/circuit.h>
#include <edacious/core/component.h>
#include <edacious/core/integration.h>
#include <edacious/core/dc.h>
#include <edacious/core/icons.h>
#include <edacious/core/scope.h>
#include <edacious/core/stamp.h>
#include <edacious/core/spice.h>
#include <edacious/core/wire.h>

#ifdef _ES_INTERNAL
# include <edacious/core/core_internal.h>
#endif

/* Flags to ES_CoreInit() */
#define ES_INIT_PRELOAD_ALL	0x01	/* Preload all installed modules */

/* Dynamically-linked module */
typedef struct es_module {
	const char *version;		/* Version number */
	const char *descr;		/* Long description */
	const char *url;		/* Package URL */
	void (*init)(const char *ver);	/* Init callback */
	void (*destroy)(void);		/* Destroy callback */
	ES_ComponentClass **comClasses;	/* Component classes */
	VG_NodeOps **vgClasses;		/* VG node classes */
} ES_Module;

__BEGIN_DECLS
extern AG_Object   esVfsRoot;		 /* General-purpose VFS */
extern void       *esCoreClasses[];	 /* Base object classes */
extern void       *esSchematicClasses[]; /* Base schematic VG classes */
extern const char *esEditableClasses[];	 /* User-editable object classes */
extern ES_Component *esModelVFS;	 /* Model library VFS */

void	ES_CoreInit(Uint);
void	ES_CoreDestroy(void);
void	ES_SetObjectOpenHandler(AG_Window *(*fn)(void *));
void	ES_SetObjectCloseHandler(void (*fn)(void *));
void	ES_InsertComponent(ES_Circuit *, VG_Tool *, ES_ComponentClass *);

int	ES_LoadModule(const char *);
int	ES_UnloadModule(const char *);

AG_Window *ES_OpenObject(void *);
void       ES_CloseObject(void *);
__END_DECLS

#include <edacious/core/core_close.h>
#endif /* _EDACIOUS_CORE_H_ */
