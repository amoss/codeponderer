#!/bin/bash
mkdir -p testcases/pp-gawk
pushd testcases/pp-gawk
# No -save-temps=cwd option on BSD. 
for t in ../gawk-4.0.2/*.c ; do gcc -c -save-temps -DDEFPATH='".:/usr/local/share/awk"' -DHAVE_CONFIG_H -DGAWK -DLOCALEDIR='"/usr/local/share/locale"' -I../gawk-4.0.2 $t ; done
# Not a top-level unit, included from regex.c. Needs right flags to preprocess out the GCC function attribute crap.
rm regexec.*
# Explicit zero-length extension also works on BSD, but needs to know the command is not a fucked up attempt at an extension.
sed -i -e 's/^# \([0-9]*\)/#line \1/' *.i
sed -i -e 's/internal_function//' *.i
popd
