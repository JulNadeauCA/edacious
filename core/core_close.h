/*	Public domain	*/

/* Undo internal header definitions */
#ifndef _ES_INTERNAL
# undef Uint
# undef Uchar
# undef Ulong
# ifdef _ES_DEFINED_CDECLS
#  undef _ES_DEFINED_CDECLS
#  undef __BEGIN_DECLS
#  undef __END_DECLS
# endif
#endif /* !_ES_INTERNAL */

#ifdef _ES_DEFINED_DECLSPEC
# undef _ES_DEFINED_DECLSPEC
# undef DECLSPEC
#endif
