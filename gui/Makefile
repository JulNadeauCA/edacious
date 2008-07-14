TOP=	..
include ${TOP}/Makefile.config

PROJECT=	"edacious"
PROJINCLUDES=	../configure.lua

PROG=		edacious
PROG_TYPE=	"GUI"
PROG_GUID=	"2dc24ff4-7e71-4a98-84db-0abc0af29030"
PROG_LINKS=	es_generic es_macro es_sources es_core \
		freesg_m ag_core ag_gui ag_sc ag_dev ag_vg \
		pthreads SDL SDLmain opengl freetype

SRCS=	main.c
LIBS=	${TOP}/generic/.libs/libes_generic.a \
	${TOP}/macro/.libs/libes_macro.a \
	${TOP}/sources/.libs/libes_sources.a \
	${TOP}/core/.libs/libes_core.a \
	${FREESG_LIBS} ${AGAR_DEV_LIBS} ${AGAR_VG_LIBS} ${AGAR_LIBS}

CFLAGS+=${FREESG_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.prog.mk
