TOP=	.
include ${TOP}/Makefile.config

PROJECT=	"edacious"
PROJINCLUDES=	configure.lua

SUBDIR=		core \
		gui \
		ecminfo \
		transient \
		generic \
		macro \
		sources

INCDIR=		core \
		generic \
		macro \
		sources

CFLAGS+=${AGAR_MATH_CFLAGS} ${AGAR_DEV_CFLAGS} ${AGAR_VG_CFLAGS} ${AGAR_CFLAGS}

all: all-subdir
clean: clean-subdir
cleandir: cleandir-config cleandir-subdir
depend: depend-subdir
regress: regress-subdir
install: install-includes install-subdir install-config
deinstall: deinstall-includes deinstall-subdir deinstall-config

configure: configure.in
	cat configure.in | mkconfigure > configure
	chmod 755 configure

cleandir-config:
	rm -fR include config 
	rm -f Makefile.config config.log configure.lua .projfiles.out .projfiles2.out
	touch Makefile.config
	find . -name premake.lua -exec rm -f {} \;

release:
	-${MAKE} cleandir
	sh mk/dist.sh commit

install-includes:
	${SUDO} ${INSTALL_INCL_DIR} ${INCLDIR}
	${SUDO} ${INSTALL_INCL_DIR} ${INCLDIR}/edacious
	@(cd include/edacious && for DIR in ${INCDIR} config; do \
	    echo "mk/install-includes.sh $$DIR ${INCLDIR}/edacious"; \
	    ${SUDO} env \
	      INSTALL_INCL_DIR="${INSTALL_INCL_DIR}" \
	      INSTALL_INCL="${INSTALL_INCL}" \
	      ${SH} ${SRCDIR}/mk/install-includes.sh \
	        $$DIR ${DESTDIR}${INCLDIR}/edacious; \
	done)

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

install-config:
	${SUDO} ${INSTALL_PROG} edacious-config "${BINDIR}"

deinstall-config:
	${SUDO} ${DEINSTALL_PROG} "${BINDIR}/edacious-config"

.PHONY: install deinstall configure release
.PHONY: install-includes deinstall-includes 
.PHONY: install-config deinstall-config

include ${TOP}/mk/build.subdir.mk
include ${TOP}/mk/build.common.mk
