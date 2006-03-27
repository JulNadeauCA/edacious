#	$Csoft: Makefile,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $

TOP=	..
include ${TOP}/Makefile.config

LIB=	component
SRCS=	component.c ground.c inverter.c resistor.c semiresistor.c \
	spst.c spdt.c selcom.c inscom.c digital.c and.c or.c led.c \
	logic_probe.c

CFLAGS+=${AGAR_SC_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/csoft.lib.mk
