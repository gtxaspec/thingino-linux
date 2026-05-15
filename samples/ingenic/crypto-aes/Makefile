CC = mips-linux-uclibc-gnu-gcc

INCLUDE_DIRS = -I./include
CFLAGS = -Wall $(INCLUDE_DIRS) -uclibc
src := $(wildcard ./*.c)
objs := $(patsubst %.c, %.o, $(src))

OUTPUT_NAME := sample_crypto_aes

$(OUTPUT_NAME): $(objs)
	$(CC) -o $@ $^
%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm $(objs) $(OUTPUT_NAME)
