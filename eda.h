/*	Public domain	*/

#ifndef _AGAR_EDA_H_
#define _AGAR_EDA_H_

#include <config/debug.h>

#include <agar/core/snprintf.h>
#include <agar/core/vsnprintf.h>
#include <agar/core/vasprintf.h>

#define Fatal AG_FatalError

#include <config/enable_nls.h>
#ifdef ENABLE_NLS
# include <libintl.h>
# define _(String) dgettext("cadtools",String)
# ifdef dgettext_noop
#  define N_(String) dgettext_noop("cadtools",String)
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

#include <math.h>
#include <string.h>

#include <circuit/circuit.h>
#include <component/ground.h>
#include <sources/vsource.h>
#include <icons.h>

#ifdef WIN32
#define ES_PATHSEPC '\\'
#define ES_PATHSEP "\\"
#else
#define ES_PATHSEPC '/'
#define ES_PATHSEP "/"
#endif

__BEGIN_DECLS
AG_Window *ES_CreateEditionWindow(void *);
__END_DECLS

#endif /* _AGAR_EDA_H_ */
