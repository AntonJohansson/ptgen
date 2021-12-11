#!/bin/sh

gcc src/ptgen.c -O3 -g -o ptgen
./ptgen test
dot -Tpng ast.dot -o ast.png
latexmk -pdf ast.tex
