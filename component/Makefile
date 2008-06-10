TOP=	..
include ${TOP}/Makefile.config

LIB=		es_component
LIB_SHARED=	Yes
LIB_GUID=	"4235f5a4-78e3-4afe-9bf7-e944ef017d1f"
LIB_LINKS=	ag_core pthreads SDL opengl freetype

SRCS=	component.c ground.c inverter.c resistor.c semiresistor.c \
	spst.c spdt.c digital.c and.c or.c led.c logic_probe.c wire.c

CFLAGS+=${AGAR_SC_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.lib.mk
