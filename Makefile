PSPSDK	= $(shell psp-config --pspsdk-path)

TARGET_LIB = ./build/lib/libgaas.a

OBJS		= source/vram.o source/imageloader.o source/callback.o source/lighting.o source/gwdloader.o\
				source/drawtext.o source/graphics.o source/mp3audio.o source/common.o source/objloader.o\
				source/ctrl.o source/collision.o source/wavaudio.o source/glTFLoader.o source/lodepng/lodepng.o\

INCDIR		= 
CFLAGS		= -O2 -G0 -Wall -fcommon
CXXFLAGS	= $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS	= $(CFLAGS)

LIBDIR		= ./ include $(PSPSDK)/../lib
LIBS		=	-ljpeg -lpng -lz -lpspvram -lpspgum_vfpu -lpspgu -lpspvfpu\
				-lpsputility -lm -lpspaudio -lpspaudiocodec -lpspmp3 -lpsprtc -lpsppower\
				
LDFLAGS	=

include $(PSPSDK)/lib/build.mak

font.h : font.rgba
	bin2c font.rgba source/font.h Font

fallback.h : fallback.rgba
	bin2c fallback.rgba source/fallback.h Fallback

all2:
ifeq (,$(wildcard ./build))
	mkdir ./build
	mkdir ./build/lib
	mkdir ./build/include
endif
	make font.h
	make fallback.h
	cp ./source/*.h ./build/include/
	make
	cp ./build/include/*.h ./samples/common/
	cp ./build/lib/libgaas.a ./samples/common/
