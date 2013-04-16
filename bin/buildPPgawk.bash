#!/bin/bash
mkdir -p testcases/pp-gawk
pushd testcases/pp-gawk
for t in ../gawk-4.0.2/*.c ; do gcc -c -save-temps=cwd -DDEFPATH='".:/usr/local/share/awk"' -DHAVE_CONFIG_H -DGAWK -DLOCALEDIR='"/usr/local/share/locale"' -I../gawk-4.0.2 $t ; done
sed -i 's/^# \([0-9]*\)/#line \1/' *.i
popd
