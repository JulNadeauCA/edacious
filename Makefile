TOP=	.
include ${TOP}/Makefile.config

PROJECT=	"agar-eda"
PROJINCLUDES=	configure.lua

SUBDIR=		Schematics \
		core \
		generic \
		macro \
		measurement \
		schem \
		sources

PROG=		eda
PROG_TYPE=	"GUI"
PROG_GUID=	"2dc24ff4-7e71-4a98-84db-0abc0af29030"
PROG_LINKS=	es_generic es_macro es_measurement es_sources \
		es_schem es_core \
		freesg_m ag_core ag_gui ag_sc ag_dev ag_vg \
		pthreads SDL SDLmain opengl freetype

SRCS=	main.c
LIBS+=	generic/.libs/libes_generic.a \
	macro/.libs/libes_macro.a \
	measurement/.libs/libes_measurement.a \
	sources/.libs/libes_sources.a \
	schem/.libs/libes_schem.a \
	core/.libs/libes_core.a \
	${FREESG_LIBS} ${AGAR_DEV_LIBS} ${AGAR_VG_LIBS} ${AGAR_LIBS}
CFLAGS+=${FREESG_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

all: all-subdir ${PROG}

configure: configure.in
	cat configure.in | mkconfigure > configure
	chmod 755 configure

release: cleandir
	sh mk/dist.sh commit

.PHONY: configure release

include ${TOP}/mk/build.prog.mk
