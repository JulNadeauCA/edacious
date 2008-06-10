TOP=	.
include ${TOP}/Makefile.config

PROJECT=	"agar-eda"
PROJINCLUDES=	configure.lua

SUBDIR=	schem \
	Schematics

PROG=		eda
PROG_TYPE=	"GUI"
PROG_GUID=	"2dc24ff4-7e71-4a98-84db-0abc0af29030"
PROG_LINKS=	es_schem \
		ag_core ag_gui ag_sc ag_dev \
		pthreads SDL SDLmain opengl freetype

SRCS=	and.c \
	circuit.c \
	component.c \
	dc.c \
	digital.c \
	ground.c \
	insert_tool.c \
	inverter.c \
	led.c \
	logic_probe.c \
	main.c \
	or.c \
	resistor.c \
	scope.c \
	select_tool.c \
	semiresistor.c \
	sim.c \
	spdt.c \
	spice.c \
	spst.c \
	vsine.c \
	vsource.c \
	vsquare.c \
	wire.c \
	wire_tool.c

LIBS+=	schem/.libs/libes_schem.a

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
