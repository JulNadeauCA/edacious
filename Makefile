TOP=	.
include ${TOP}/Makefile.config

SUBDIR=	component \
	circuit \
	sources \
	schem

PROG=	eda
SRCS=	eda.c

LIBS+=	circuit/.libs/libcircuit.a \
	component/.libs/libcomponent.a \
	sources/.libs/libsources.a \
	schem/.libs/libschem.a \

LIBS+=	${AGAR_SC_LIBS} ${AGAR_DEV_LIBS} ${AGAR_VG_LIBS} ${AGAR_LIBS}
CFLAGS+=${AGAR_SC_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

all: all-subdir ${PROG}

configure: configure.in
	cat configure.in | mkconfigure > configure
	chmod 755 configure

release: cleandir
	sh mk/dist.sh commit

.PHONY: configure release

include ${TOP}/mk/build.prog.mk
