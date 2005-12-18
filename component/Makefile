#	$Csoft: Makefile,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $

TOP=	..
include ${TOP}/Makefile.config

LIB=	component
SRCS=	component.c conductor.c ground.c inverter.c resistor.c semiresistor.c \
	spst.c spdt.c vsource.c selcom.c inscom.c digital.c

CFLAGS+=${AGAR_SC_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/csoft.lib.mk
