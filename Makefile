#	$Csoft: Makefile,v 1.2 2005/09/08 09:48:56 vedge Exp $

TOP=	.
include ${TOP}/Makefile.config

SUBDIR=	component \
	circuit

PROG=	eda
SRCS=	eda.c

CFLAGS+=${AGAR_SC_CFLAGS} ${AGAR_CFLAGS}
LIBS+=	circuit/libcircuit.a \
	component/libcomponent.a \
	${AGAR_SC_LIBS} ${AGAR_LIBS}

all: all-subdir ${PROG}

configure: configure.in
	cat configure.in | manuconf > configure
	chmod 755 configure

release: cleandir
	sh mk/dist.sh commit

.PHONY: configure release

include ${TOP}/mk/csoft.prog.mk
