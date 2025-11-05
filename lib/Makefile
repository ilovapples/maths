CVI_PATH := ../cvi
CHASHMAP_PATH := ../c-hashmap
CFLAGS := -I../incl -I$(CVI_PATH) -I$(CHASHMAP_PATH) -Wall -Wextra -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-folding-constant -std=c23 -O3 -g
LDFLAGS := -O3 -g -fPIC

CC := cc

SHARED := 

SRC := $(wildcard *.c)
OUT := $(patsubst %.c,%.o,$(SRC))

all: $(OUT)

%.o: %.c Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $(SHARED) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(OUT)
