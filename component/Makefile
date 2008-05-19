TOP=	..
include ${TOP}/Makefile.config

LIB=	component
SRCS=	component.c ground.c inverter.c resistor.c semiresistor.c \
	spst.c spdt.c digital.c and.c or.c led.c logic_probe.c wire.c

CFLAGS+=${AGAR_SC_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.lib.mk
