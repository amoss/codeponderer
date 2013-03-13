#!/bin/bash
# sed is a bunch of crap on BSD deriviatives and needs multiline commands that
# cannot be written portably in the Makefile. This wrapper hides that pain.

sed -e '1i\
symbols = {' -e 's/^\([A-Z_0-9]\+\)=\([0-9]\+\)$/\2:"\1",/' -e "s/'\(.*\)'=\([0-9]\+\)/\2:'\1',/" -e'$a\
-1:"dummy"}' $1
