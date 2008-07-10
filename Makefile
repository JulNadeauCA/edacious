TOP=	.
include ${TOP}/Makefile.config

PROJECT=	"edacious"
PROJINCLUDES=	configure.lua

SUBDIR=		edacious-config \
		core \
		generic \
		macro \
		measurement \
		sources \
		Schematics

INCDIRS=	core \
		generic \
		macro \
		measurement \
		sources

PROG=		edacious
PROG_TYPE=	"GUI"
PROG_GUID=	"2dc24ff4-7e71-4a98-84db-0abc0af29030"
PROG_LINKS=	es_generic es_macro es_measurement es_sources es_core \
		freesg_m ag_core ag_gui ag_sc ag_dev ag_vg \
		pthreads SDL SDLmain opengl freetype

SRCS=	main.c
LIBS+=	generic/.libs/libes_generic.a \
	macro/.libs/libes_macro.a \
	measurement/.libs/libes_measurement.a \
	sources/.libs/libes_sources.a \
	core/.libs/libes_core.a \
	${FREESG_LIBS} ${AGAR_DEV_LIBS} ${AGAR_VG_LIBS} ${AGAR_LIBS}

CFLAGS+=${FREESG_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

all: all-subdir ${PROG}
clean: clean-config clean-subdir
cleandir: cleandir-config cleandir-subdir
install: install-subdir install-includes
deinstall: deinstall-subdir
depend: depend-subdir
regress: regress-subdir

configure: configure.in
	cat configure.in | mkconfigure > configure
	chmod 755 configure

release: cleandir
	sh mk/dist.sh commit

install-includes:
	${SUDO} ${INSTALL_INCL_DIR} ${INCLDIR}
	${SUDO} ${INSTALL_INCL_DIR} ${INCLDIR}/edacious
	@if [ "${SRC}" != "" ]; then \
		(cd ${SRC} && for DIR in ${INCDIRS}; do \
		    echo "mk/install-includes.sh $$DIR"; \
		    ${SUDO} env \
		        INSTALL_INCL_DIR="${INSTALL_INCL_DIR}" \
		        INSTALL_INCL="${INSTALL_INCL}" \
		        ${SH} mk/install-includes.sh $$DIR ${INCLDIR}/edacious; \
		done); \
		echo "mk/install-includes.sh config"; \
		${SUDO} env \
		    INSTALL_INCL_DIR="${INSTALL_INCL_DIR}" \
		    INSTALL_INCL="${INSTALL_INCL}" \
		    ${SH} mk/install-includes.sh config ${INCLDIR}/edacious; \
		${SUDO} ${INSTALL_INCL} ${SRC}/eda.h \
		    ${INCLDIR}/edacious/eda.h; \
	else \
		for DIR in ${INCDIRS} config; do \
		    echo "mk/install-includes.sh $$DIR"; \
		    ${SUDO} env \
		    INSTALL_INCL_DIR="${INSTALL_INCL_DIR}" \
		    INSTALL_INCL="${INSTALL_INCL}" \
		    ${SH} mk/install-includes.sh $$DIR ${INCLDIR}/edacious; \
		done; \
		${SUDO} ${INSTALL_INCL} eda.h ${INCLDIR}/edacious/eda.h; \
	fi

.PHONY: clean cleandir install deinstall depend regress
.PHONY: configure cleandir-config package snapshot release
.PHONY: install-includes

include ${TOP}/mk/build.prog.mk
