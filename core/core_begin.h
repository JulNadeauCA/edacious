/*	Public domain	*/

/* Internal header definitions */
#define Uint unsigned int
#define Uchar unsigned char
#define Ulong unsigned long

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

/*
 * Expand "DECLSPEC" to any compiler-specific keywords, as required for proper
 * visibility of symbols in shared libraries.
 * See: http://gcc.gnu.org/wiki/Visibility
 */
#ifndef DECLSPEC
# if defined(__BEOS__)
#  if defined(__GNUC__)
#   define DECLSPEC	__declspec(dllexport)
#  else
#   define DECLSPEC	__declspec(export)
#  endif
# elif defined(__WIN32__)
#  ifdef __BORLANDC__
#   ifdef _ES_INTERNAL
#    define DECLSPEC 
#   else
#    define DECLSPEC	__declspec(dllimport)
#   endif
#  else
#   define DECLSPEC	__declspec(dllexport)
#  endif
# elif defined(__OS2__)
#  ifdef __WATCOMC__
#   ifdef _ES_INTERNAL
#    define DECLSPEC	__declspec(dllexport)
#   else
#    define DECLSPEC
#   endif
#  else
#   define DECLSPEC
#  endif
# else
#  if defined(__GNUC__) && __GNUC__ >= 4
#   define DECLSPEC	__attribute__ ((visibility("default")))
#  else
#   define DECLSPEC
#  endif
# endif
# define _ES_DEFINED_DECLSPEC
#endif
#ifdef __SYMBIAN32__ 
# ifndef EKA2 
#  undef DECLSPEC
#  define DECLSPEC
#  define _ES_DEFINED_DECLSPEC
# elif !defined(__WINS__)
#  undef DECLSPEC
#  define DECLSPEC __declspec(dllexport)
#  define _ES_DEFINED_DECLSPEC
# endif
#endif
