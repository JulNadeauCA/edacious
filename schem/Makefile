TOP=	..
include ${TOP}/Makefile.config

LIB=		es_schem
LIB_SHARED=	Yes
LIB_GUID=	"83c8ebcf-e06d-48f5-8591-d16ec2340bf6"
LIB_LINKS=	ag_core pthreads SDL opengl freetype

SRCS=	schem.c select_tool.c line_tool.c circle_tool.c point_tool.c \
	proximity_tool.c text_tool.c port.c port_tool.c wire.c block.c

CFLAGS+=${AGAR_SC_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.lib.mk
