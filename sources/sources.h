/*	Public domain	*/

#include <edacious/core/core_begin.h>

#include <edacious/sources/isource.h>
#include <edacious/sources/vsource.h>

#include <edacious/sources/varb.h>
#include <edacious/sources/vsine.h>
#include <edacious/sources/vsquare.h>
#include <edacious/sources/vsweep.h>

__BEGIN_DECLS
extern void *esSourcesClasses[];
void ES_SourcesInit(void);
void ES_SourcesDestroy(void);
__END_DECLS

#include <edacious/core/core_close.h>
