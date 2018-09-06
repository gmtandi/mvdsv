#!/bin/bash
set -e
scp build_$1/mvdsv$2 huetf@nl.andi.com.br:~/htdocs/build/mvdsv_$1$2

exit 0
