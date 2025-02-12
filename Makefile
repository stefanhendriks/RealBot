CPP = g++
ARCHFLAG = -m32

META_DIR = ./dependencies/metamod-hl1/metamod
HLSDK_DIR = ./dependencies/hlsdk

BASEFLAGS = -Dstricmp=strcasecmp -Dstrcmpi=strcasecmp -Dlinux=1
CPPFLAGS = ${BASEFLAGS} ${ARCHFLAG} -O2 -mtune=generic -march=i686 -mmmx -msse -msse2 -O2 -mfpmath=sse -s \
        -Wno-write-strings -Wno-attributes -std=gnu++14 -static-libstdc++ -shared-libgcc \
        -I"${META_DIR}" -I"${HLSDK_DIR}/common" -I"${HLSDK_DIR}/dlls" \
        -I"${HLSDK_DIR}/engine" -I"${HLSDK_DIR}/pm_shared" -I"${HLSDK_DIR}/public"

OBJ = NodeMachine.o \
	bot.o \
	bot_buycode.o \
	bot_client.o \
	bot_func.o \
	bot_navigate.o \
	build.o \
	dll.o \
	engine.o \
	game.o \
	util.o \
	ChatEngine.o \
	IniParser.o

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
SO_SUFFIX = so
endif
ifeq ($(UNAME_S),Darwin)
SO_SUFFIX = dylib
endif

realbot_mm.${SO_SUFFIX}: ${OBJ}
	${CPP} ${ARCHFLAG} -fPIC -shared -o $@ ${OBJ} -ldl
	mkdir -p Release
	mv $@ ${OBJ} Release

clean:
	rm -f *.o
	rm -f *.map
	rm -f *.${SO_SUFFIX}
	mv Release/*.${SO_SUFFIX} .
	rm -f Release/*
	mv *.${SO_SUFFIX} Release

distclean:
	rm -rf Release
	mkdir -p Release

%.o:	%.cpp
	${CPP} ${CPPFLAGS} -c $< -o $@

%.o:	%.c
	${CPP} ${CPPFLAGS} -c $< -o $@
