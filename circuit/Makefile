TOP=	..
include ${TOP}/Makefile.config

LIB=		es_circuit
LIB_SHARED=	Yes
LIB_GUID=	"659a36aa-9996-4aaf-ab4a-8c2587bfbc25"
LIB_LINKS=	ag_core pthreads SDL opengl freetype

SRCS=	sim.c circuit.c spice.c dc.c scope.c insert_tool.c select_tool.c \
	wire_tool.c

CFLAGS+=${AGAR_SC_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.lib.mk
