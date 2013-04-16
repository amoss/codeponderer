#!/bin/bash

for name in testcases/pp-gawk/*.i ;
do
  BASE=$(basename $name)
  STEM=${BASE%.*}
  awk '/^#line/ { if($3=="\"../gawk-4.0.2/'$STEM'.c\"") Inside=1; else Inside=0; } {if(Inside) print;}'  testcases/pp-gawk/$STEM.i >testcases/pp-gawk/$STEM.ii
done
