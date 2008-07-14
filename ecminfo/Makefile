TOP=	..
include Makefile.config

PROG=		ecminfo
PROG_TYPE=	"CLI"
PROG_GUID=	"0646155d-c9d7-49d2-9a03-f130ff5ccd6b"
PROG_LINKS=	edacious freesg ag_gui ag_core \
		pthreads SDL SDLmain opengl freetype

SRCS=	ecminfo.c
#MAN1=	ecminfo.1

all: ${PROG}

configure: configure.in
	cat configure.in | mkconfigure > configure
	chmod 755 configure

CFLAGS+=${EDACIOUS_CFLAGS} ${FREESG_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}
LIBS+=	${EDACIOUS_LIBS} ${FREESG_LIBS} ${AGAR_VG_LIBS} ${AGAR_LIBS}

.PHONY: configure

include ${TOP}/mk/build.prog.mk
include ${TOP}/mk/build.man.mk
