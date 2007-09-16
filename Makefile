#	$Csoft: Makefile,v 1.2 2005/09/08 09:48:56 vedge Exp $

TOP=	.
include ${TOP}/Makefile.config

SUBDIR=	component \
	circuit \
	sources

PROG=	eda
SRCS=	eda.c

LIBS+=	circuit/libcircuit.a \
	component/libcomponent.a \
	sources/libsources.a

LIBS+=	${AGAR_SC_LIBS} ${AGAR_DEV_LIBS} ${AGAR_VG_LIBS} ${AGAR_LIBS}
CFLAGS+=${AGAR_SC_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

all: all-subdir ${PROG}

configure: configure.in
	cat configure.in | mkconfigure > configure
	chmod 755 configure

release: cleandir
	sh mk/dist.sh commit

.PHONY: configure release

include ${TOP}/mk/csoft.prog.mk
