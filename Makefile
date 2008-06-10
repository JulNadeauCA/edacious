TOP=	.
include ${TOP}/Makefile.config

PROJECT=	"agar-eda"
PROJINCLUDES=	configure.lua

SUBDIR=	component \
	circuit \
	sources \
	schem \
	Schematics

PROG=		eda
PROG_TYPE=	"GUI"
PROG_GUID=	"2dc24ff4-7e71-4a98-84db-0abc0af29030"
PROG_LINKS=	ag_core ag_gui ag_sc ag_dev \
		pthreads SDL SDLmain opengl freetype

SRCS=		eda.c

LIBS+=	circuit/.libs/libes_circuit.a \
	component/.libs/libes_component.a \
	sources/.libs/libes_sources.a \
	schem/.libs/libes_schem.a

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
