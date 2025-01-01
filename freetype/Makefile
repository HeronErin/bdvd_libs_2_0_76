#!/bin/sh

############################################################################
#      THIS MUST APPEAR AT THE TOP OF THE MAKEFILE.  DO NOT MODIFY.
#vvvv#####################vvvvvvvvvvvvvvvvvvvvvvv#####################vvvv##
include ../../bdvd_top.mak
#^^^^#####################^^^^^^^^^^^^^^^^^^^^^^^#####################^^^^##



#	make BUILD_DIR=`pwd`/builds/unix PLATFORM="unix" all; make


TARGET             ?= freetype-2.3.4
TARGET_DIR         := ${TARGET}_${BDVD_ARCH}


TARGET_DIR_MARKER  := ${TARGET_DIR}/.empty_file
TARGET_PATCH_FILE  := ${TARGET}.${BDVD_ARCH}.patch
TARGET_PATCH_MARKER:= ${TARGET_DIR}/.target_patch_marker

CPPFLAGS           += -I${BDVD_STAGE_DIR}/usr/local/include
LDFLAGS            += -L${BDVD_STAGE_DIR}/usr/local/lib

CROSS              := ${BDVD_ARCH}

DIRECTFB_INSTALL_PATH := ${BDVD_STAGE_DIR}

ifeq (y,${DMALLOC})
  DMALLOC_COMMAND = sed -ie '26 s/^/\#undef malloc\n\#undef free\n\#undef alloc\n\#undef realloc\n/' ${TARGET_DIR}/src/base/ftsystem.c ; sed -ie '16 s/^/\#undef realloc\n\#undef free\n/' ${TARGET_DIR}/src/base/ftutil.c
else 
  DMALLOC_COMMAND = true
endif


export CFLAGS CPPFLAGS LDFLAGS CC CPP CXX LD AR RANLIB STRIP DIRECTFB_INSTALL_PATH


all:   ${TARGET_DIR}/Makefile ${TARGET_PATCH_MARKER}
	${MAKE} -C ${TARGET_DIR} all

${TARGET_DIR_MARKER}: ${TARGET}.tar.gz
	tar xzvf ${TARGET}.tar.gz
	mv ${TARGET} ${TARGET_DIR}
	touch ${TARGET_DIR_MARKER}

${TARGET_DIR}/Makefile: ${TARGET_DIR_MARKER} ${TARGET_PATCH_FILE}

${TARGET_PATCH_MARKER}: ${TARGET_PATCH_FILE} 
	-patch -d ${TARGET_DIR} -Np1 < ${TARGET_PATCH_FILE} 
	cd ${TARGET_DIR} && ./configure --enable-shared --enable-static --build=i386-pc-linux-gnu --host=${BDVD_ARCH}-gnu
	${DMALLOC_COMMAND}
	touch ${TARGET_PATCH_MARKER}

prep0: ${TARGET_DIR_MARKER}

prep1: ${TARGET_DIR}/Makefile ${TARGET_PATCH_MARKER}

install: ${TARGET_DIR}/Makefile ${TARGET_PATCH_MARKER}
	DESTDIR=${BDVD_STAGE_DIR} ${MAKE} -C ${TARGET_DIR} install

quick: all
	@echo SKIPPING MAKE INSTALL OF ${TARGET}!

clean: 
	rm -f ${TARGET_PATCH_MARKER}
	if [ -f ${TARGET_DIR}/Makefile ] ; then DESTDIR=${BDVD_STAGE_DIR} ${MAKE} -C ${TARGET_DIR} clean ; fi

distclean:
	if [ -f ${TARGET_DIR}/Makefile ] ; then DESTDIR=${BDVD_STAGE_DIR} ${MAKE} -C ${TARGET_DIR} distclean ; fi
	rm -fr ${TARGET_DIR}


############################################################################
#                      DO NOT MODIFY ANYTHING BELOW
#vvvv#####################vvvvvvvvvvvvvvvvvvvvvvv#####################vvvv##
include ../../bdvd_btm.mak
#^^^^#####################^^^^^^^^^^^^^^^^^^^^^^^#####################^^^^##

