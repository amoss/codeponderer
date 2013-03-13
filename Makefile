frontends: generated/cInCParser.c generated/cInPyParser.py

frontends/cInPy.g: frontends/c.g
	sed -e's/REPLACELANG/Python/' -e's/REPLACENAME/cInPy/' -e's/REPLACEHIDDEN/{$$channel=HIDDEN}/' frontends/c.g >frontends/cInPy.g

frontends/cInC.g: frontends/c.g
	sed -e's/REPLACELANG/C/' -e's/REPLACENAME/cInC/' -e's/REPLACEHIDDEN/{$$channel=HIDDEN;}/' frontends/c.g >frontends/cInC.g

generated/cInPyParser.py: frontends/cInPy.g antlr-3.1.3/lib/antlr-3.1.3.jar
	java -classpath antlr-3.1.3/lib/antlr-3.1.3.jar org.antlr.Tool -fo generated frontends/cInPy.g

generated/cInCParser.c: frontends/cInC.g antlr-3.1.3/lib/antlr-3.1.3.jar
	java -classpath antlr-3.1.3/lib/antlr-3.1.3.jar org.antlr.Tool -fo generated frontends/cInC.g

antlr-3.1.3/lib/antlr-3.1.3.jar:
	curl http://www.antlr3.org/download/antlr-3.1.3.tar.gz | tar xz
