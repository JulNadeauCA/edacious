/*	Public domain	*/

#ifndef _AGAR_EDA_H_
#define _AGAR_EDA_H_

#include <config/debug.h>
#include <config/enable_nls.h>
#ifdef ENABLE_NLS
# include <libintl.h>
# define _(String) dgettext("agar-eda",String)
# ifdef dgettext_noop
#  define N_(String) dgettext_noop("agar-eda",String)
# else
#  define N_(String) (String)
# endif
#else
# undef _
# undef N_
# undef ngettext
# define _(s) (s)
# define N_(s) (s)
# define ngettext(Singular,Plural,Number) ((Number==1)?(Singular):(Plural))
#endif
#undef ENABLE_NLS

#include <agar/core.h>
#include <agar/gui.h>
#include <agar/vg.h>
#include <agar/sc.h>

#include <agar/config/_mk_have_unsigned_typedefs.h>
#ifndef _MK_HAVE_UNSIGNED_TYPEDEFS
#define Uchar unsigned char
#define Uint unsigned int
#define Ulong unsigned long
#endif

#include <schem/schem.h>
#include <component/component.h>
#include <circuit/circuit.h>

#include <icons.h>

#include <math.h>
#include <string.h>

#ifdef WIN32
# define ES_PATHSEP '\\'
#else
# define ES_PATHSEP '/'
#endif

#ifndef MIN
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define	MAX(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef MIN3
#define	MIN3(a,b,c) MIN((a),MIN((b),(c)))
#endif
#ifndef MAX3
#define	MAX3(a,b,c) MAX((a),MAX((b),(c)))
#endif

#define Fatal AG_FatalError
#define Snprintf AG_Snprintf
#define Vsnprintf AG_Vsnprintf

__BEGIN_DECLS
extern AG_Object vfsRoot;

extern void *esComponentClasses[];
extern const void *esSchematicClasses[];
extern void *esBaseClasses[];
extern const char *esEditableClasses[];

AG_Window *ES_CreateEditionWindow(void *);
void       ES_CloseEditionWindow(void *);
__END_DECLS

#endif /* _AGAR_EDA_H_ */
