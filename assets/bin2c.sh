#!/bin/sh
infile="$1"
basename=$(basename "$infile")
name="${basename%.*}"
xxd -i "$infile" > "./assets/xxd_out/${name}.c"
xxd -i "$infile" | sed 's/^unsigned/extern unsigned/' | sed 's/ =.*$//' > "./assets/xxd_out/${name}.h"