#!/bin/bash

pushd antlr-3.1.3/runtime/C
autoreconf -ivf
aclocal
autoconf
autoheader
automake --add-missing
automake
./configure --prefix=`pwd`/../../../build --enable-64bit
make 
make install
popd
