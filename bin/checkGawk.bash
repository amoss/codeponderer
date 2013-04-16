#!/bin/bash
for t in testcases/pp-gawk/*.i ; do demos/trivial $t 2>&1 | grep error | grep -o '[a-z_.]*([0-9]*)' ; done | sort -u | tr '()' ' ,' | awk '{a[$1]=a[$1]" "$2;}END{for(i in a)print i,a[i]}'
