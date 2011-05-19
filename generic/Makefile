TOP=..
include ${TOP}/Makefile.config

LIBDIR=		${MODULEDIR}
LIB=		generic
LIB_INSTALL=	Yes
LIB_SHARED=	Yes
LIB_MODULE=	Yes
LIB_GUID=	"368455fb-0b23-4ab6-ba37-6d17c8f33a81"
LIB_LINKS=	es_core ag_gui ag_core_static \
		pthreads SDL opengl freetype

SRCS=	generic.c \
	capacitor.c \
	diode.c \
	ground.c \
	inductor.c \
	led.c \
	nmos.c \
	npn.c \
	pmos.c \
	pnp.c \
	resistor.c \
	semiresistor.c \
	spdt.c \
	spst.c

CFLAGS+=${AGAR_MATH_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.lib.mk
include ${TOP}/mk/build.man.mk
