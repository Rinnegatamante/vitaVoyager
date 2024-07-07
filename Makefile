TARGET		:= vitaVoyager
TITLE		:= VOYAGERQ3
GIT_VERSION := $(shell git describe --abbrev=6 --dirty --always --tags)

SOURCES  := code/renderercommon code/qcommon code/botlib code/client code/server code/renderergl1 code/psp2 code/sys
INCLUDES := code/renderercommon code/qcommon code/botlib code/client code/server code/renderergl1 code/psp2 code/sys

LIBS = -lvitaGL -lvitashark -lvorbisfile -lvorbis -logg  -lspeexdsp -lmpg123 -lSceAppMgr_stub \
	-lc -lSceCommonDialog_stub -lSceAudio_stub -lSceLibKernel_stub -lSceShaccCgExt -ltaihen_stub \
	-lSceNet_stub -lSceNetCtl_stub -lpng -lz -lSceDisplay_stub -lSceGxm_stub \
	-Wl,--whole-archive -lSceSysmodule_stub -Wl,--no-whole-archive -lSceCtrl_stub -lSceTouch_stub -lSceMotion_stub -lm \
	-lSceAppUtil_stub -lScePgf_stub -ljpeg -lSceRtc_stub -lScePower_stub -lcurl \
	-lssl -lcrypto -lSceSsl_stub -lmathneon -lvitashark -lSceShaccCg_stub -lSceKernelDmacMgr_stub
	

CFILES   := $(filter-out code/psp2/psp2_dll_hacks.c,$(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c)))
CPPFILES   := $(foreach dir,$(CPPSOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

export INCLUDE	:= $(foreach dir,$(INCLUDES),-I$(dir))

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = $(INCLUDE) -D__PSP2__ -D__FLOAT_WORD_ORDER=1 -D__GNU__ -DRELEASE \
        -DUSE_ICON -DARCH_STRING=\"arm\" -DBOTLIB -DUSE_CODEC_VORBIS -DUSE_CURL=1 \
        -DPRODUCT_VERSION=\"1.36_GIT_ba68b99c-2018-01-23\" -DHAVE_VM_COMPILED=true \
        -mfpu=neon -mcpu=cortex-a9 -fsigned-char -DELITEFORCE \
        -Wl,-q,--no-enum-size-warning -O3 -g -ffast-math -fno-short-enums
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

exec-only: eboot.bin

$(TARGET).vpk: $(TARGET).velf
	vita-make-fself -s $< build/eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE) -d ATTRIBUTE2=12 "$(TARGET)" param.sfo

	vita-pack-vpk -s param.sfo -b build/eboot.bin $(TARGET).vpk \
		-a build/sce_sys/icon0.png=sce_sys/icon0.png \
		-a build/sce_sys/livearea/contents/bg.png=sce_sys/livearea/contents/bg.png \
		-a build/sce_sys/livearea/contents/startup.png=sce_sys/livearea/contents/startup.png \
		-a build/sce_sys/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml

eboot.bin: $(TARGET).velf
	vita-make-fself -s $< eboot.bin
	
%.velf: %.elf
	cp $< $<.unstripped.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS) $(TARGET).elf.unstripped.elf $(TARGET).vpk build/eboot.bin ./param.sfo
