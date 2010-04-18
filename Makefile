TOP=	.
include ${TOP}/Makefile.config

PROJECT=	"edacious"
PROJINCLUDES=	configure.lua

SUBDIR=		core \
		generic \
		macro \
		sources \
		edacious-config \
		gui \
		ecminfo \
		transient

INCPUB=		core generic macro sources
INCDIRS=	core generic macro sources

CFLAGS+=${AGAR_MATH_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

all: all-subdir
clean: clean-subdir
cleandir: cleandir-subdir
depend: depend-subdir
regress: regress-subdir
install: install-includes install-subdir
deinstall: deinstall-includes deinstall-subdir

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
include ${TOP}/mk/build.man.mk
