TOP=..
include ${TOP}/Makefile.config

LIB=		es_sources
LIB_INSTALL=	Yes
LIB_SHARED=	Yes
LIB_GUID=	"ae639d3f-9b93-464c-8d47-e9d3f0c2176e"
LIB_LINKS=	es_core ag_gui ag_core_static \
		pthreads SDL opengl freetype

SRCS=	isource.c \
	varb.c \
	vsine.c \
	vsource.c \
	vsquare.c \
	vsweep.c

CFLAGS+=${FREESG_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.lib.mk
include ${TOP}/mk/build.man.mk
