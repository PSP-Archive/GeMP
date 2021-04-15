#DEFS += -DUSE_GPU
#DEFS += -DDEBUG

PSPSDK=$(shell psp-config --pspsdk-path)

PSPDEV=$(shell psp-config --pspdev-path)/bin
INCLUDES=$(PSPSDK)/include

PBP = EBOOT.PBP
BINARY = out
OBJS = startup.o GB/cpu.o GB/gb.o GB/lcd.o GB/sgb.o \
	GB/rom.o GB/mbc.o GB/apu.o GB/cheat.o \
	main.o pg.o renderer.o menu.o filer.o sound.o saveload.o image.o gz.o \
	syscall.o oggplayer.o music_gui.o utils.o mp3.o
LIBS = lib/unziplib.a lib/libz.a\
	lib/libmad.a lib/libvorbisidec.a lib/libpspaudio.a lib/libpspaudiolib.a \
	lib/libpng.a lib/libjpeg.a \
	-lc
	
all: $(PBP)

$(PBP): $(BINARY)
	outpatch
	mksfo "GeMP 3.3" param.sfo
	pack-pbp eboot.pbp param.sfo ICON01.png NULL NULL NULL NULL outp NULL
	rm param.sfo outp out
	cp EBOOT.PBP G:\PSP\GAME\GeMP\EBOOT.PBP

$(BINARY): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@
	$(STRIP) $(BINARY)

%.o: %.c
	$(CC) -I $(INCLUDES) $(DEFS) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) -I $(INCLUDES) $(DEFS) $(ARCHFLAGS) -c $< -o $@

cleansmall:
	$(RM) *.map out outp *.o GB/*.o INC/*.o
	
clean:
	$(RM) *.map out outp *.o GB/*.o INC/*.o *.bak GB/*.bak INC/*.bak

include Makefile.psp
