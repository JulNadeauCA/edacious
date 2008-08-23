/*	Public domain	*/

/* Internal header definitions */
#undef _MK_HAVE_UNSIGNED_TYPEDEFS
#include <edacious/config/_mk_have_unsigned_typedefs.h>
#ifndef _MK_HAVE_UNSIGNED_TYPEDEFS
# define _MK_HAVE_UNSIGNED_TYPEDEFS
# define Uint unsigned int
# define Uchar unsigned char
# define Ulong unsigned long
#endif

#if !defined(__BEGIN_DECLS) || !defined(__END_DECLS)
# define _ES_DEFINED_CDECLS
# if defined(__cplusplus)
#  define __BEGIN_DECLS extern "C" {
#  define __END_DECLS   }
# else
#  define __BEGIN_DECLS
#  define __END_DECLS
# endif
#endif
