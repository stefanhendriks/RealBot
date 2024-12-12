# CPP must be g++ on Linux
CPP = g++

ARCHFLAG = i686

METAMOD_SRCDIR = ../../metamod-p/metamod

HLSDK_BASEDIR = ../../hlsdk-2.3-p4

BASEFLAGS = -g -Wall
CPPFLAGS = ${BASEFLAGS} -march=${ARCHFLAG} -O2 -w -I"${METAMOD_SRCDIR}" -I"${HLSDK_BASEDIR}/multiplayer/common" -I"${HLSDK_BASEDIR}/multiplayer/dlls" -I"${HLSDK_BASEDIR}/multiplayer/engine" -I"${HLSDK_BASEDIR}/multiplayer/pm_shared" -I"${HLSDK_BASEDIR}/multiplayer/public"

all: Bsp2Rbn DumpNodes DrawNodes

Bsp2Rbn: Bsp2Rbn.o bspfile.o world.o mathlib.o entity.o cmdlib.o scriplib.o ../NodeMachine.o ../bot_func.o ../bot_navigate.o dummy.o trace.o util.o build.o
	g++ -o Bsp2Rbn Bsp2Rbn.o bspfile.o world.o mathlib.o entity.o cmdlib.o scriplib.o ../NodeMachine.o ../bot_func.o ../bot_navigate.o dummy.o trace.o util.o build.o

DrawNodes: DrawNodes.o build.o
	${CPP} -O2 -o DrawNodes DrawNodes.o build.o

DumpNodes: DumpNodes.o build.o
	${CPP} -O2 -o DumpNodes DumpNodes.o build.o

dist:	DumpNodes DrawNodes Bsp2Rbn
	cp DumpNodes DrawNodes Bsp2Rbn /home/evyncke/public_html/cs/realbot
clean:
	rm -f *.o *.bmp *.rbn *.rbx
	rm -f *.map
	rm -f *.so
	rm -f Bsp2Rbn DumpNodes DrawNodes

%.o:	%.cpp
	${CPP} ${CPPFLAGS} -c $< -o $@

%.o:	%.c
	${CPP} ${CPPFLAGS} -c $< -o $@

