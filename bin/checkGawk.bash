#!/bin/bash
for t in testcases/pp-gawk/*.ii ; 
do 
  echo -n "$t "
  demos/trivial $t 2>&1 | grep ERROR | grep -o '([0-9]*)' | tr -d '()' | sort -un | tr '\n' ','; 
  echo
done # | sort -u # | tr '()' ' ,' # | awk '{a[$1]=a[$1]" "$2;}END{for(i in a)print i,a[i]}'
