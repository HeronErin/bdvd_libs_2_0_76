include $(ROOTDIR)/config.arch

TOOL_DIR_PREFIX=$(shell dirname $(shell which $(CC)))/..
UCLIBC_INC = $(ROOTDIR)/uClibc/include
UCLIBC_LIB = $(ROOTDIR)/uClibc/lib
ifneq (,$(findstring linux,$(CROSS)))
	TARGET=$(CROSS:linux-=linux)
	TARGET_ARCH ?= $(CROSS:-linux-=)
else
	ifneq (,$(findstring uclibc,$(CROSS)))
		TARGET_ARCH ?= $(CROSS:-uclibc-=)
		TARGET=$(TARGET_ARCH)-linux
	else
		echo "FIXME: Cannot determine target architecture"
		exit 99
	endif
endif

all:

clean:

distclean: clean

romfs:
	mkdir -p $(ROMFSDIR)/usr/wlan ;
	$(ROMFSINST) -e CONFIG_USER_WLAN_BCM43XX_USB bcmdl /usr/wlan/bcmdl
	$(ROMFSINST) -e CONFIG_USER_WLAN_BCM43XX_USB rtecdc.trx /usr/wlan/rtecdc.trx
	$(ROMFSINST) -e CONFIG_USER_WLAN_BCM43XX_USB wl /usr/wlan/wl
	$(ROMFSINST) -e CONFIG_USER_WLAN_BCM43XX_USB wl_usb.ko /usr/wlan/wl_usb.ko
	$(ROMFSINST) -e CONFIG_USER_WLAN_BCM43XX_USB wlan_bcm43xx_usb.rc /usr/wlan/wlan_bcm43xx_usb.rc
	$(ROMFSINST) -e CONFIG_USER_WLAN_BCM43XX_USB wpsenr /usr/wlan/wpsenr
