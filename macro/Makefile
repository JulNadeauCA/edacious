TOP=..
include ${TOP}/Makefile.config

LIB=		es_macro
LIB_INSTALL=	Yes
LIB_SHARED=	Yes
LIB_GUID=	"2ae2fd4c-0377-4bc3-aac5-3db97644fc6d"
LIB_LINKS=	es_core ag_gui ag_core_static \
		pthreads SDL opengl freetype

SRCS=	and_gate.c \
	digital.c \
	inverter.c \
	or_gate.c

CFLAGS+=${FREESG_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.lib.mk
include ${TOP}/mk/build.man.mk
