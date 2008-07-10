/*	Public domain	*/

#ifndef _EDACIOUS_EDACIOUS_H_
#define _EDACIOUS_EDACIOUS_H_

#ifdef _ES_INTERNAL
# include <config/debug.h>
# include <config/enable_nls.h>
# ifdef ENABLE_NLS
#  include <libintl.h>
#  define _(String) dgettext("edacious",String)
#  ifdef dgettext_noop
#   define N_(String) dgettext_noop("edacious",String)
#  else
#   define N_(String) (String)
#  endif
# else
#  undef _
#  undef N_
#  undef ngettext
#  define _(s) (s)
#  define N_(s) (s)
#  define ngettext(Singular,Plural,Number) ((Number==1)?(Singular):(Plural))
# endif
# undef ENABLE_NLS
#endif /* _ES_INTERNAL */

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>
#include <freesg/m/m.h>
#include <freesg/m/m_gui.h>

#include <agar/config/_mk_have_unsigned_typedefs.h>
#ifndef _MK_HAVE_UNSIGNED_TYPEDEFS
#define Uchar unsigned char
#define Uint unsigned int
#define Ulong unsigned long
#endif

#if !defined(__BEGIN_DECLS) || !defined(__END_DECLS)
# if defined(__cplusplus)
#  define __BEGIN_DECLS	extern "C" {
#  define __END_DECLS	}
# else
#  define __BEGIN_DECLS
#  define __END_DECLS
# endif
#endif

/* -------------------------------------------------------------------------- */

#include <core/schem.h>
#include <core/component.h>
#include <core/circuit.h>
#include <core/icons.h>
#include <core/stamp.h>

#ifdef _ES_INTERNAL

#include <math.h>
#include <string.h>

# ifdef _WIN32
#  define PATHSEP '\\'
# else
#  define PATHSEP '/'
# endif
# ifndef MIN
#  define MIN(a,b) (((a)<(b))?(a):(b))
# endif
# ifndef MAX
#  define MAX(a,b) (((a)>(b))?(a):(b))
# endif
# ifndef MIN3
#  define MIN3(a,b,c) MIN((a),MIN((b),(c)))
# endif
# ifndef MAX3
#  define MAX3(a,b,c) MAX((a),MAX((b),(c)))
# endif

# define Fatal AG_FatalError
# define Snprintf AG_Snprintf
# define Vsnprintf AG_Vsnprintf

#endif /* _ES_INTERNAL */

__BEGIN_DECLS
extern AG_Object vfsRoot;

extern void *esComponentClasses[];
extern const void *esSchematicClasses[];
extern void *esBaseClasses[];
extern const char *esEditableClasses[];

AG_Window *ES_CreateEditionWindow(void *);
void       ES_CloseEditionWindow(void *);
__END_DECLS

#endif /* _EDACIOUS_EDACIOUS_H_ */
