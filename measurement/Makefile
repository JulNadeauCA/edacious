TOP=..
include ${TOP}/Makefile.config

LIB=		es_measurement
LIB_INSTALL=	Yes
LIB_SHARED=	Yes
LIB_GUID=	"378ddcbd-4416-459e-a69f-fc47201315d8"
LIB_LINKS=	es_core ag_gui ag_core_static \
		pthreads SDL opengl freetype

SRCS=	logic_probe.c \
	scope.c

CFLAGS+=${FREESG_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.lib.mk
include ${TOP}/mk/build.man.mk
