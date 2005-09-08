#	$Csoft: Makefile,v 1.1.1.1 2005/09/08 05:26:55 vedge Exp $

TOP=	.
include ${TOP}/Makefile.config

SUBDIR=	tools \
	component \
	circuit

PROG=	aeda
SRCS=	aeda.c tools.c

CFLAGS+=${AGAR_CFLAGS}
LIBS+=	tools/libtools.a \
	component/libcomponent.a \
	circuit/libcircuit.a \
	${AGAR_LIBS}

all: all-subdir ${PROG}

configure: configure.in
	cat configure.in | manuconf > configure
	chmod 755 configure

release: cleandir
	sh mk/release.sh

.PHONY: configure release

include ${TOP}/mk/csoft.prog.mk
