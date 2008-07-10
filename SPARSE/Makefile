TOP=..
include ${TOP}/Makefile.config

LIB=		es_sparse
LIB_INSTALL=	Yes
LIB_SHARED=	Yes
LIB_GUID=	"0c09acdb-225a-4b7f-a8f9-edc0045ae18d" /* TODO generate a new one */
LIB_LINKS=	ag_gui ag_core_static \
		pthreads SDL opengl freetype

SRCS=	spallocate.c \
	spbuild.c \
	spfactor.c \
	spfortran.c \
	spoutput.c \
	spsolve.c \
	sptest.c \
	sputils.c 


CFLAGS+=${FREESG_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

include ${TOP}/mk/build.lib.mk
include ${TOP}/mk/build.man.mk
