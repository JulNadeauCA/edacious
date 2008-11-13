/*	Public domain	*/

/* Internal code definitions */
#include <config/debug.h>
#include <config/enable_nls.h>
#ifdef ENABLE_NLS
# include <libintl.h>
# define _(String) dgettext("edacious",String)
# ifdef dgettext_noop
#  define N_(String) dgettext_noop("edacious",String)
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

#ifdef _WIN32
# define PATHSEP "\\"
# define PATHSEPCHAR '\\'
#else
# define PATHSEP "/"
# define PATHSEPCHAR '/'
#endif

#ifndef MIN
# define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
# define MAX(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef MIN3
# define MIN3(a,b,c) MIN((a),MIN((b),(c)))
#endif
#ifndef MAX3
# define MAX3(a,b,c) MAX((a),MAX((b),(c)))
#endif

#define Fatal AG_FatalError
#define Snprintf AG_Snprintf
#define Vsnprintf AG_Vsnprintf
