all: frontends demos

CINCS=-Iantlr-3.1.3/runtime/C/include -Iantlr-3.1.3/runtime/C 
CLIBS=-lantlr3c -Lantlr-3.1.3/runtime/C/installdir/lib 
demos: demos/trivial

demos/trivial: demos/trivial.cc generated/cInCParser.o generated/cInCLexer.o
	g++ demos/trivial.cc generated/cInCLexer.o generated/cInCParser.o -Igenerated ${CINCS} -o demos/trivial ${CLIBS}

frontends: generated/cInCParser.c generated/cInPyParser.py

frontends/cInPy.g: frontends/c.g
	sed -e's/REPLACELANG/Python/' -e's/REPLACENAME/cInPy/' -e's/REPLACEHIDDEN/{$$channel=HIDDEN}/' frontends/c.g >frontends/cInPy.g

frontends/cInC.g: frontends/c.g
	sed -e's/REPLACELANG/C/' -e's/REPLACENAME/cInC/' -e's/REPLACEHIDDEN/{$$channel=HIDDEN;}/' frontends/c.g >frontends/cInC.g

generated/cInPyParser.py: frontends/cInPy.g antlr-3.1.3/lib/antlr-3.1.3.jar
	java -classpath antlr-3.1.3/lib/antlr-3.1.3.jar org.antlr.Tool -fo generated frontends/cInPy.g

generated/cInCParser.c: frontends/cInC.g antlr-3.1.3/lib/antlr-3.1.3.jar
	java -classpath antlr-3.1.3/lib/antlr-3.1.3.jar org.antlr.Tool -fo generated frontends/cInC.g

generated/cInCLexer.o: generated/cInCLexer.c
	gcc -c generated/cInCLexer.c -Igenerated -Iantlr-3.1.3/runtime/C/include -Iantlr-3.1.3/runtime/C -o generated/cInCLexer.o

generated/cInCParser.o: generated/cInCParser.c
	gcc -c generated/cInCParser.c -Igenerated -Iantlr-3.1.3/runtime/C/include -Iantlr-3.1.3/runtime/C -o generated/cInCParser.o

antlr-3.1.3/lib/antlr-3.1.3.jar:
	curl http://www.antlr3.org/download/antlr-3.1.3.tar.gz | tar xz
