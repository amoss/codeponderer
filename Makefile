all: frontends demos

SYS=$(shell uname)
CINCS=-Ibuild/include -Iantlr-3.1.3/runtime/C 
ifeq ($(SYS),Darwin)
CLIBS=-lantlr3c -Lbuild/lib
RUNLIB=build/lib/libantlr3c.la
endif
ifeq ($(SYS),Linux)
CLIBS=-static -lantlr3c -Lbuild/lib
RUNLIB=build/lib/libantlr3c.so
endif

demos: demos/trivial

demos/trivial: demos/trivial.cc generated/cInCParser.o generated/cInCLexer.o models/util.o models/c.o cRuntime
	g++ -g demos/trivial.cc generated/cInCLexer.o generated/cInCParser.o models/util.o models/c.o -I. -Igenerated ${CINCS} -o demos/trivial ${CLIBS}

frontends: generated/cInCParser.c generated/cInPyParser.py

frontends/cInPy.g: frontends/c.g
	sed -e's/REPLACELANG/Python/' -e's/REPLACENAME/cInPy/' -e's/REPLACEHIDDEN/{$$channel=HIDDEN}/' frontends/c.g >frontends/cInPy.g

frontends/cInC.g: frontends/c.g
	sed -e's/REPLACELANG/C/' -e's/REPLACENAME/cInC/' -e's/REPLACEHIDDEN/{$$channel=HIDDEN;}/' frontends/c.g >frontends/cInC.g

models/c.o: models/c.cc models/c.h
	g++ -c models/c.cc -I. -Igenerated -Iantlr-3.1.3/runtime/C/include -Iantlr-3.1.3/runtime/C -o models/c.o

models/util.o: models/util.h models/util.cc
	g++ -c models/util.cc -I. -Igenerated -Iantlr-3.1.3/runtime/C/include -Iantlr-3.1.3/runtime/C -o models/util.o

generated/cInPyParser.py: frontends/cInPy.g antlr-3.1.3/lib/antlr-3.1.3.jar
	java -classpath antlr-3.1.3/lib/antlr-3.1.3.jar org.antlr.Tool -fo generated frontends/cInPy.g

generated/cInCParser.c: frontends/cInC.g antlr-3.1.3/lib/antlr-3.1.3.jar
	java -classpath antlr-3.1.3/lib/antlr-3.1.3.jar org.antlr.Tool -fo generated frontends/cInC.g

generated/cInCLexer.o: generated/cInCLexer.c cRuntime
	gcc -c generated/cInCLexer.c -Igenerated -Iantlr-3.1.3/runtime/C/include -Iantlr-3.1.3/runtime/C -o generated/cInCLexer.o

generated/cInCParser.o: generated/cInCParser.c cRuntime
	gcc -c generated/cInCParser.c -Igenerated -Iantlr-3.1.3/runtime/C/include -Iantlr-3.1.3/runtime/C -o generated/cInCParser.o

antlr-3.1.3/lib/antlr-3.1.3.jar:
	curl http://www.antlr3.org/download/antlr-3.1.3.tar.gz | tar xz

cRuntime: antlr-3.1.3/lib/antlr-3.1.3.jar $(RUNLIB)
$(RUNLIB): 
	bin/buildCRuntime.bash
