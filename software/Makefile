include $(MSCDIR)/software/common.mak

OBJECTS=isr.o main.o

all: tester.bin tester.fbi

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	chmod -x $@

%.fbi: %.bin
	$(MSCDIR)/tools/mkmscimg $< write $@

tester.elf: $(OBJECTS) libs

%.elf:
	$(LD) $(LDFLAGS) \
		-T $(MSCDIR)/software/libbase/linker-sdram.ld \
		-N -o $@ \
		$(MSCDIR)/software/libbase/crt0.o \
		$(OBJECTS) \
		-L$(MSCDIR)/software/libbase \
		-L$(MSCDIR)/software/libcompiler-rt \
		-lbase -lcompiler-rt
	chmod -x $@

main.o: main.c
	$(compile-dep)

%.o: %.c
	$(compile-dep)

%.o: %.S
	$(assemble)

flash: tester.fbi
	xc3sprog -c papilio -I$(FLASH_PROXY) tester.fbi:w:0x70000:BIN

libs:
	$(MAKE) -C $(MSCDIR)/software/libcompiler-rt
	$(MAKE) -C $(MSCDIR)/software/libbase

clean:
	$(RM) $(OBJECTS) $(OBJECTS:.o=.d) tester.elf tester.bin .*~ *~

.PHONY: all main.o clean libs flash
