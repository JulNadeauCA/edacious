TOP=	.
include ${TOP}/Makefile.config

PROJECT=	"edacious"
PROJINCLUDES=	configure.lua

SUBDIR=		core \
		generic \
		macro \
		sources \
		Schematics \
		edacious-config \
		gui

INCPUB=		core generic macro sources
INCDIRS=	core generic macro sources

LIB=		edacious
LIB_INSTALL=	Yes
LIB_SHARED=	Yes
LIB_GUID=	"49370965-fa94-4c11-b0e2-edcf46034884"
LIB_LINKS=	freesg_m ag_core ag_gui ag_sc ag_dev ag_vg \
		pthreads SDL SDLmain opengl freetype

LIBS=	generic/libes_generic.la \
	macro/libes_macro.la \
	sources/libes_sources.la \
	core/libes_core.la

CFLAGS+=${FREESG_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

all: all-subdir lib${LIB}.a lib${LIB}.la
clean: clean-subdir
cleandir: cleandir-subdir
depend: depend-subdir
regress: regress-subdir
install: install-lib install-includes install-subdir
deinstall: deinstall-lib deinstall-includes deinstall-subdir

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
		        ${SH} mk/install-includes.sh $$DIR \
			${INCLDIR}/edacious; \
		done); \
		echo "mk/install-includes.sh config"; \
		${SUDO} env \
		    INSTALL_INCL_DIR="${INSTALL_INCL_DIR}" \
		    INSTALL_INCL="${INSTALL_INCL}" \
		    ${SH} mk/install-includes.sh config ${INCLDIR}/edacious; \
		(cd ${SRC} && for INC in ${INCPUB}; do \
		    ${SUDO} ${INSTALL_INCL} ${SRC}/$$INC/$$INC.h \
		        ${INCLDIR}/edacious/$$INC.h; \
		done); \
	else \
		for DIR in ${INCDIRS} config; do \
		    echo "mk/install-includes.sh $$DIR"; \
		    ${SUDO} env \
		    INSTALL_INCL_DIR="${INSTALL_INCL_DIR}" \
		    INSTALL_INCL="${INSTALL_INCL}" \
		    ${SH} mk/install-includes.sh $$DIR ${INCLDIR}/edacious; \
		done; \
		(for INC in ${INCPUB}; do \
		    ${SUDO} ${INSTALL_INCL} $$INC/$$INC.h \
		        ${INCLDIR}/edacious/$$INC.h; \
		done); \
	fi

deinstall-includes:
	${FIND} . -type f -name '*.h' -print \
	    | ${AWK} '{print "${DEINSTALL_INCL} ${INCLDIR}/edacious/"$$1}' \
	    | ${SUDO} ${SH}
	@if [ "${SRC}" != "" ]; then \
		echo "${FIND} ${SRC} -type f -name '*.h' -print \
		    | ${AWK} '{print "${DEINSTALL_INCL} \
		    ${INCLDIR}/edacious/"$$1}' \
		    | ${SUDO} ${SH}"; \
		(cd ${SRC} && ${FIND} . -type f -name '*.h' -print \
		    | ${AWK} '{print "${DEINSTALL_INCL} \
		    ${INCLDIR}/edacious/"$$1}' \
		    | ${SUDO} ${SH}); \
	fi

.PHONY: install deinstall configure release install-includes deinstall-includes 

include ${TOP}/mk/build.lib.mk
