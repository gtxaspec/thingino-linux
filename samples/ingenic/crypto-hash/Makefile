CC = mips-linux-gnu-gcc

INCLUDE_DIRS = -I./include
CFLAGS = $(INCLUDE_DIRS) -Wall
src := stream_hash.c
objs := $(patsubst %.c, %.o, $(src))

CFLAGS += -muclibc

OUTPUT_NAME := sample_crypto_hash

$(OUTPUT_NAME): $(objs)
	$(CC) $(CFLAGS) -o $@ $^
%.o:%.c
	$(CC) -c -o $@ $<

.PHONY: clean
clean:
	rm $(objs) $(OUTPUT_NAME)
