TOP=	.
include ${TOP}/Makefile.config

PROJECT=	"agar-eda"
PROJINCLUDES=	configure.lua

SUBDIR=		Schematics

PROG=		eda
PROG_TYPE=	"GUI"
PROG_GUID=	"2dc24ff4-7e71-4a98-84db-0abc0af29030"
PROG_LINKS=	freesg_m \
		ag_core ag_gui ag_sc ag_dev ag_vg \
		pthreads SDL SDLmain opengl freetype

SRCS=	and.c \
	capacitor.c \
	circuit.c \
	component.c \
	dc.c \
	digital.c \
	diode.c \
	ground.c \
	inductor.c \
	insert_tool.c \
	interpreteur.c \
	inverter.c \
	isource.c \
	led.c \
	logic_probe.c \
	main.c \
	nmos.c \
	or.c \
	pmos.c \
	resistor.c \
	schem.c \
	schem_block.c \
	schem_circle_tool.c \
	schem_line_tool.c \
	schem_point_tool.c \
	schem_port.c \
	schem_port_tool.c \
	schem_proximity_tool.c \
	schem_select_tool.c \
	schem_text_tool.c \
	schem_wire.c \
	schem_wire_tool.c \
	scope.c \
	semiresistor.c \
	sim.c \
	spdt.c \
	spice.c \
	spst.c \
	stamp.c \
	varb.c\
	vsine.c \
	vsource.c \
	vsquare.c \
	vsweep.c \
	wire.c

LIBS+=	${FREESG_LIBS} ${AGAR_DEV_LIBS} ${AGAR_VG_LIBS} ${AGAR_LIBS}
CFLAGS+=${FREESG_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

all: all-subdir ${PROG}

configure: configure.in
	cat configure.in | mkconfigure > configure
	chmod 755 configure

release: cleandir
	sh mk/dist.sh commit

.PHONY: configure release

include ${TOP}/mk/build.prog.mk
