#!/usr/bin/env sh
./mml -E 'x=(-$B+~csqrt{$B^2-4*$A*$C})/(2$A); print{x.0, x.1};' --set_var:A="$1" --set_var:B="$2" --set_var:C="$3"
