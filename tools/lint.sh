#!/bin/sh
# Coding rules from the brief: no float, no malloc, fixed arrays only,
# printing and other PC-only code only in platform_pc.c.
status=0
if grep -nE '\b(float|double)\b|\b(malloc|calloc|realloc|free)[[:space:]]*\(' src/*.c src/*.h; then
    echo "lint: float or dynamic allocation found"; status=1
fi
others=$(ls src/*.c src/*.h | grep -v platform_pc.c)
if grep -nE '#include[[:space:]]*<(stdio|stdlib|string)\.h>|\b(printf|rand|srand)[[:space:]]*\(' $others; then
    echo "lint: PC-only code outside platform_pc.c"; status=1
fi
[ $status -eq 0 ] && echo "lint: ok"
exit $status
