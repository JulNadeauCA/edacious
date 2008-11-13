TOP=..
include ${TOP}/Makefile.config

LIB=		es_core
LIB_INSTALL=	Yes
LIB_SHARED=	Yes
LIB_GUID=	"0c09acdb-225a-4b7f-a8f9-edc0045ae18d"
LIB_LINKS=	ag_gui ag_core_static \
		pthreads SDL opengl freetype

SRCS=	core.c \
	circuit.c \
	circuit_edit.c \
	component.c \
	component_edit.c \
	dc.c \
	insert_tool.c \
	interpreteur.c \
	scope.c \
	sim.c \
	spice.c \
	wire.c \
	wire_tool.c \
	schem.c \
	schem_block.c \
	schem_port.c \
	schem_port_tool.c \
	schem_select_tool.c \
	schem_wire.c \
	integration.c \
	tools.c \
	library.c

CFLAGS+=${AGAR_MATH_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.lib.mk
include ${TOP}/mk/build.man.mk
