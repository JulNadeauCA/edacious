TOP=	..
include ${TOP}/Makefile.config

LIB=		es_sources
LIB_SHARED=	Yes
LIB_GUID=	"de92e373-2b51-4011-9032-eb8dbcb65e9a"
LIB_LINKS=	ag_core pthreads SDL opengl freetype

SRCS=	vsource.c vsquare.c vsine.c

CFLAGS+=${AGAR_SC_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.lib.mk
