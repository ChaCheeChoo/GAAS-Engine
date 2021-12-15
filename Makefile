PSPSDK	= $(shell psp-config --pspsdk-path)

TARGET_LIB = ./build/lib/libgaas.a

OBJS		= source/vram.o source/font.c source/imageloader.o source/callback.o source/lighting.o\
				source/drawtext.o source/graphics.o source/mp3audio.o source/billboard.o source/common.o\
				source/ctrl.o source/collision.o source/wavaudio.o source/gwd.o source/objloader.o\

INCDIR		= 
CFLAGS		= -O2 -G0 -Wall
CXXFLAGS	= $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS	= $(CFLAGS)

LIBDIR		= ./ include $(PSPSDK)/../lib
LIBS		=	-ljpeg -lpng -lz -lpspvram -lpspgum_vfpu -lpspgu -lpspvfpu\
				-lpsputility -lm -lpspaudio -lpspaudiocodec -lpspmp3 -lpsprtc -lpsppower\
				
LDFLAGS	=

include $(PSPSDK)/lib/build.mak

font.c : font.raw
	bin2c font.raw source/font.c Font

all2:
ifeq (,$(wildcard ./build))
	mkdir ./build
	mkdir ./build/lib
	mkdir ./build/include
endif
	make font.c
	cp ./source/*.h ./build/include/
	make
	cp ./build/include/*.h ./samples/common/
	cp ./build/lib/libgaas.a ./samples/common/
