# Wear-levelling project. Plain C11 on a PC.
#
#   make                 build ./wearlevel with trace.h
#   make TRACE=more_traces/trace_hot95.h
#   make run             print the CSV rows for baseline, array and heap
#   make test            unit tests plus a full run, all with checks
#   make sanitize        the same tests under AddressSanitizer and UBSan
#   make lint            check the coding rules (no float, no malloc, stdio only in platform_pc.c)
#   make crosscheck      compare the C program with the Python reference model

CC      ?= gcc
TRACE   ?= trace.h
OUT     ?= wearlevel
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
INCLUDE  = -Isrc -I.

CORE   = src/memory.c src/baseline.c src/wl.c src/fb_array.c src/fb_heap.c
SIM    = src/sim.c src/platform_pc.c

all: $(OUT)

$(OUT): $(CORE) $(SIM) $(wildcard src/*.h) $(TRACE)
	$(CC) $(CFLAGS) $(INCLUDE) -DTRACE_FILE=\"$(TRACE)\" -o $@ $(CORE) $(SIM)

test_fb: $(CORE) tests/test_fb.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $(CORE) tests/test_fb.c

test_wl: $(CORE) tests/test_wl.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $(CORE) tests/test_wl.c

run: $(OUT)
	./$(OUT) --header --spares $${SPARES:-4}

test: test_fb test_wl $(OUT)
	./test_fb
	./test_wl
	./$(OUT) --header --spares 4

sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -g -fsanitize=address,undefined -fno-sanitize-recover=all" test

lint:
	./tools/lint.sh

crosscheck: $(OUT)
	python3 tools/crosscheck.py $(TRACE) $(or $(SPARES),1,4,16,64,256) ./$(OUT)

clean:
	rm -f wearlevel test_fb test_wl

.PHONY: all run test sanitize lint crosscheck clean
