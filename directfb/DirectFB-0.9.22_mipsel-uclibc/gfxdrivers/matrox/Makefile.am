## Makefile.am for DirectFB/src/core/gfxcards/matrox

INCLUDES = \
	-I$(top_srcdir)/include	\
	-I$(top_builddir)/lib	\
	-I$(top_srcdir)/lib	\
	-I$(top_srcdir)/src	\
	-I$(top_srcdir)/systems

matrox_LTLIBRARIES = libdirectfb_matrox.la

if BUILD_STATIC
matrox_DATA = $(matrox_LTLIBRARIES:.la=.o)
endif

matroxdir = $(MODULEDIR)/gfxdrivers

libdirectfb_matrox_la_SOURCES =	\
	i2c-dev.h		\
	matrox.c		\
	matrox.h		\
	matrox_3d.c		\
	matrox_3d.h		\
	matrox_bes.c		\
	matrox_crtc2.c		\
	matrox_maven.c		\
	matrox_maven.h		\
	matrox_screen_crtc2.c	\
	matrox_spic.c		\
	matrox_state.c		\
	matrox_state.h		\
	regs.h			\
	mmio.h

libdirectfb_matrox_la_LDFLAGS = \
	$(SYSFS_LIBS)		\
	-export-dynamic		\
	-avoid-version		\
	$(DFB_LDFLAGS)

libdirectfb_matrox_la_LIBADD = -lm


include $(top_srcdir)/rules/libobject.make

