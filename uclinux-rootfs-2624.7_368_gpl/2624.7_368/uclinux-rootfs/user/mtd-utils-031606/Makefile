# -*- sh -*-

# $Id: Makefile,v 1.20 2001/09/05 00:18:11 andersen Exp $

#
# These flags are used for build native versions of the tools for the
# host system so it can generate images etc
#
include $(ROOTDIR)/$(LINUXDIR)/.config
ifeq ($(CONFIG_CPU_LITTLE_ENDIAN),y)
	TARGET_ENDIAN = __MIPSEL__
else
	TARGET_ENDIAN = __MIPSEB__
endif
MY_CFLAGS = -I. -I$(ROOTDIR)/$(LINUXDIR)/include -D $(TARGET_ENDIAN)
BUILD_CFLAGS += $(MY_CFLAGS)
BUILD_CC = gcc
#CFLAGS += -I. -I$(ROOTDIR)/$(LINUXDIR)/include $(INCZ) -D__USE_BSD
CFLAGS += -I. $(INCZ) -D__USE_BSD
LDPATH += -L$(ROOTDIR)/lib/zlib

TARGETS = flash_erase flash_eraseall flash_lock flash_unlock flash_info mtd_debug flashcp \
	jffs2dump norcleanmark #jffs2reader
HOST_TARGETS = jffs2dump
ifneq ($(CONFIG_FTL),)
TARGETS += ftl_format ftl_check
endif
ifneq ($(CONFIG_NFTL),)
TARGETS += nftl_format nftldump
endif
ifneq ($(CONFIG_MTD_BRCMNAND),)
TARGETS +=  nandwrite nandinfo nanddump
HOST_TARGETS += nanddump nandinfo nandwrite
endif
ifneq ($(CONFIG_JFFS_FS),)
TARGETS += mkfs.jffs
HOST_TARGETS += mkfs.jffs
endif
ifneq ($(CONFIG_JFFS2_FS),)
TARGETS += mkfs.jffs2
HOST_TARGETS += mkfs.jffs2
endif
ifneq ($(CONFIG_JFFS2_SUMMARY),)
TARGETS += sumtool
HOST_TARGETS += sumtool
endif
ifneq ($(CONFIG_MTD_DOCPROBE),)
TARGETS += doc_loadbios 
else
ifneq ($(CONFIG_MTD_DOC1000),)
TARGETS += doc_loadbios
endif
endif

BUILD_TARGETS = $(patsubst %,build/%,$(HOST_TARGETS))

ZINCS = zlib.h zconf.h 

FSINCS = nodelist.h

#SYMLINKS = compr_rtime.c compr_rubin.c compr.c pushpull.h histo_mips.h compr_rubin.h compr_zlib.c
SYMLINKS = $(ZINCS) $(FSINCS)

UTILS = brcmnand_util.c
UTILS_OBJS = $(UTILS:.o=.c)


JFFS2_OBJS = crc32.o compr_rtime.o mkfs.jffs2.o compr_zlib.o compr.o


BUILD_JFFS2_OBJS = $(patsubst %,build/%,$(JFFS2_OBJS))

all: $(SYMLINKS) build $(TARGETS) $(BUILD_TARGETS)

.PHONY:
build:
	[ -d build ] || mkdir build

clean:
	rm -f *.o *.gdb $(TARGETS) $(BUILD_TARGETS) $(SYMLINKS)
	rm -rf build

romfs:
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASE        /bin/flash_info
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASE        /bin/flash_erase
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASE -s /bin/flash_erase /bin/erase
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASEALL     /bin/flash_eraseall
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASEALL     /bin/norcleanmark
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_ERASEALL -s /bin/flash_eraseall /bin/eraseall
ifneq ($(CONFIG_FTL),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FTL_CHECK    /bin/ftl_check
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_FTL_FORMAT   /bin/ftl_format
endif
ifneq ($(CONFIG_NFTL),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NFTLDUMP     /bin/nftldump
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NFTL_FORMAT  /bin/nftl_format
endif
ifneq ($(CONFIG_MTD_BRCMNAND),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDDUMP     /bin/nanddump
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDTEST     /bin/nandtest
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDWRITE    /bin/nandwrite
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_NANDINFO     /bin/nandinfo
endif
ifneq ($(CONFIG_JFFS_FS),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS     /bin/mkfs.jffs
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS     /bin/mtd_debug
endif
ifneq ($(CONFIG_JFFS2_FS),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS2    /bin/mkfs.jffs2
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_MKFSJFFS2    /bin/mtd_debug
endif
ifneq ($(CONFIG_JFFS2_SUMMARY),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTIL_MKFSJFFS2     /bin/sumtool
endif
ifneq ($(CONFIG_MTD_DOCPROBE),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_DOC_LOADBIOS /bin/doc_loadbios
#	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_DOC_LOADIPL  /bin/doc_loadipl
endif
ifneq ($(CONFIG_MTD_DOC1000),)
	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_DOC_LOADBIOS /bin/doc_loadbios
#	$(ROMFSINST) -e CONFIG_USER_MTDUTILS_DOC_LOADIPL  /bin/doc_loadipl
endif


$(FSINCS):
	ln -sf $(ROOTDIR)/$(LINUXDIR)/fs/jffs2/$@ $@
	
$(ZINCS):
	ln -sf $(ROOTDIR)/lib/zlib/$@ $@

$(JFFS2_OBJS): $(ZINCS)

mkfs.jffs2: $(JFFS2_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDPATH) -lz $(LDLIBS)

sumtool: sumtool.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDPATH) -lz $(LDLIBS)

build/mkfs.jffs2: $(BUILD_JFFS2_OBJS)
	$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $^ -lz

flash_eraseall: crc32.o flash_eraseall.o $(UTILS_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

norcleanmark: crc32.o norcleanmark.o $(UTILS_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

#build/flash_eraseall: build/crc32.o build/flash_eraseall.o
#	$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $^


jffs2reader: jffs2reader.c
	$(CC) $(LDFLAGS) jffs2reader.c -o jffs2reader $(LDPATH) -lz $(LDLIBS)

build/jffs2reader: jffs2reader.c
	$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $< -lz

jffs2dump: jffs2dump.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

build/jffs2dump: build/jffs2dump.o build/crc32.o
	$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $^

build/sumtool: build/sumtool.o build/crc32.o
	$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $^

build/%.o: %.c
	$(BUILD_CC) $(BUILD_CFLAGS) -c -o $@ $<

build/%: %.c
	$(BUILD_CC) $(BUILD_CFLAGS) -o $@ $<

