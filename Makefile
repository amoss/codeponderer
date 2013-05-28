all: frontends models demos testcases

SYS=$(shell uname)
CINCS=-Ibuild/include -Iantlr-3.1.3/runtime/C 
ifeq ($(SYS),Darwin)
CLIBS=-lantlr3c -Lbuild/lib
RUNLIB=build/lib/libantlr3c.la
GCC=gcc-mp-4.7
GPP=g++-mp-4.7
endif
ifeq ($(SYS),Linux)
CLIBS=-static -lantlr3c -Lbuild/lib
RUNLIB=build/lib/libantlr3c.so
GCC=gcc
GPP=g++
endif

CFLAGS=-g -I. -Igenerated -Iantlr-3.1.3/runtime/C/include -Iantlr-3.1.3/runtime/C

demos: demos/trivial demos/buildConfig
demos/trivial: demos/trivial.cc generated/cInCParser.o generated/cInCLexer.o models misc  $(RUNLIB)
	${GPP} -g demos/trivial.cc generated/cInCLexer.o generated/cInCParser.o $(MODOBJS) $(MISCOBJS) -I. -Igenerated ${CINCS} -o demos/trivial ${CLIBS}
demos/buildConfig: demos/buildConfig.cc generated/cInCParser.o generated/cInCLexer.o models misc  $(RUNLIB)
	${GPP} -g demos/buildConfig.cc generated/cInCLexer.o generated/cInCParser.o $(MODOBJS) $(MISCOBJS) -I. -Igenerated ${CINCS} -o demos/buildConfig ${CLIBS}


frontends: generated/cInCParser.c generated/cInPyParser.py
frontends/cInPy.g: frontends/c.g
	sed -e's/REPLACELANG/Python/' -e's/REPLACENAME/cInPy/' -e's/REPLACEHIDDEN/{$$channel=HIDDEN}/' frontends/c.g >frontends/cInPy.g
frontends/cInC.g: frontends/c.g
	sed -e's/REPLACELANG/C/' -e's/REPLACENAME/cInC/' -e's/REPLACEHIDDEN/{$$channel=HIDDEN;}/' frontends/c.g >frontends/cInC.g
generated/cInPyParser.py: frontends/cInPy.g antlr-3.1.3/lib/antlr-3.1.3.jar
	java -classpath antlr-3.1.3/lib/antlr-3.1.3.jar org.antlr.Tool -fo generated frontends/cInPy.g
generated/cInCParser.c: frontends/cInC.g antlr-3.1.3/lib/antlr-3.1.3.jar
	java -classpath antlr-3.1.3/lib/antlr-3.1.3.jar org.antlr.Tool -fo generated frontends/cInC.g
generated/cInCLexer.c: generated/cInCParser.c
generated/cInCLexer.o: generated/cInCLexer.c $(RUNLIB)
	${GCC} -c generated/cInCLexer.c ${CFLAGS} -o generated/cInCLexer.o
generated/cInCParser.o: generated/cInCParser.c $(RUNLIB)
	${GCC} -c generated/cInCParser.c ${CFLAGS} -o generated/cInCParser.o


MISCOBJS=generated/util.o generated/path.o
misc: $(MISCOBJS)
generated/path.o: misc/path.h misc/path.cc
	${GPP} -c misc/path.cc ${CFLAGS} -o generated/path.o
generated/util.o: misc/util.h misc/util.cc
	${GPP} -c misc/util.cc ${CFLAGS} -o generated/util.o

MODOBJS=generated/c-repr.o generated/c-build.o
models: $(MODOBJS)
generated/c-repr.o: models/c-repr.cc models/c-repr.h misc/util.h
	${GPP} -c models/c-repr.cc ${CFLAGS} -o generated/c-repr.o
generated/c-build.o: models/c-build.cc models/c-build.h generated/c-repr.o models/graph.h models/graph.cc misc/util.h
	${GPP} -c models/c-build.cc ${CFLAGS} -o generated/c-build.o


antlr-3.1.3/lib/antlr-3.1.3.jar:
	curl http://www.antlr3.org/download/antlr-3.1.3.tar.gz | tar xz

$(RUNLIB): antlr-3.1.3/lib/antlr-3.1.3.jar
	bin/buildCRuntime.bash

testcases: testcases/pp-gawk/array.ii
testcases/pp-gawk/array.i: testcases/gawk-4.0.2/array.c 
	bin/buildPPgawk.bash
testcases/pp-gawk/array.ii: testcases/pp-gawk/array.i
	bin/chompPPgawk.bash
testcases/gawk-4.0.2/array.c:
	curl ftp://ftp.gnu.org/gnu/gawk/gawk-4.0.2.tar.gz | tar xz -C testcases

clean:
	rm generated/*
