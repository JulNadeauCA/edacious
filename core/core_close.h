/*	Public domain	*/

/* Undo internal header definitions */
#ifndef _ES_INTERNAL
# ifdef _MK_HAVE_UNSIGNED_TYPEDEFS
#  undef _MK_HAVE_UNSIGNED_TYPEDEFS
#  undef Uint
#  undef Uchar
#  undef Ulong
# endif
# ifdef _ES_DEFINED_CDECLS
#  undef _ES_DEFINED_CDECLS
#  undef __BEGIN_DECLS
#  undef __END_DECLS
# endif
#endif /* !_ES_INTERNAL */
