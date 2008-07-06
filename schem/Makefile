TOP=..
include ${TOP}/Makefile.config

LIB=		es_schem
LIB_INSTALL=	Yes
LIB_SHARED=	Yes
LIB_GUID=	"2d584a7d-2896-46e2-aefe-41f1c24d8cfc"
LIB_LINKS=	es_core ag_gui ag_core_static \
		pthreads SDL opengl freetype

SRCS=	schem.c \
	schem_block.c \
	schem_port.c \
	schem_port_tool.c \
	schem_select_tool.c \
	schem_wire.c

CFLAGS+=${FREESG_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.lib.mk
include ${TOP}/mk/build.man.mk
