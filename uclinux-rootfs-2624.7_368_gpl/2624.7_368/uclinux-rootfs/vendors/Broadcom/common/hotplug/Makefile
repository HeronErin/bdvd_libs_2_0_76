
EXEC = usb.hotplug
OBJS = hotplug.o

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs: all
	$(ROMFSINST) /etc/hotplug.d/default/$(EXEC)
	[ -d $(ROMFSDIR)/etc/hotplug.d/usb ] || mkdir -p $(ROMFSDIR)/etc/hotplug.d/usb
	$(ROMFSINST) /etc/hotplug.d/usb/brcm_wifi.hotplug

clean:
	-rm -f $(EXEC) *.gdb *.elf *.o

