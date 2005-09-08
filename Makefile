#	$Csoft: Makefile,v 1.7 2004/11/23 02:33:09 vedge Exp $

TOP=	.
include ${TOP}/Makefile.config

SUBDIR=	tools \
	component \
	circuit

PROG=	agar-eda
SRCS=	agar-eda.c tools.c

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
