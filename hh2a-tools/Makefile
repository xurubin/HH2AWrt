#
# Makefile
#
CC = gcc -std=gnu89
CFLAGS = -ggdb3 -O2 -pedantic -Wall -Wextra
RM = rm -f
VPATH = .

COMMON_SRCS = parse_num.c my_file_functions.c bitsnbobs.c
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

THOMSON_KERNEL_OBJS = thomson_kernel.o
REDTABLE_OBJS = redtable.o
V1_MOST_FLASH_OBJS = v1_most_flash.o
V2_MOST_FLASH_OBJS = v2_most_flash.o
ENDIANSWAP_OBJS = endianswap.o


PROGRAMS = thomson_kernel redtable v1_most_flash v2_most_flash endianswap

ALL_CPPFLAGS = -I../linux/include/linux -I.

all: $(PROGRAMS)
	strip $(PROGRAMS)

%.o: %.c $(XZ_HEADERS)
	$(CC) $(ALL_CPPFLAGS) $(CFLAGS) -c -o $@ $<

v1_most_flash: $(COMMON_OBJS) $(V1_MOST_FLASH_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(COMMON_OBJS) $(V1_MOST_FLASH_OBJS)

v2_most_flash: $(COMMON_OBJS) $(V2_MOST_FLASH_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(COMMON_OBJS) $(V2_MOST_FLASH_OBJS)

thomson_kernel: $(COMMON_OBJS) $(THOMSON_KERNEL_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(COMMON_OBJS) $(THOMSON_KERNEL_OBJS)

redtable: $(COMMON_OBJS) $(REDTABLE_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(COMMON_OBJS) $(REDTABLE_OBJS)

endianswap: $(COMMON_OBJS) $(ENDIANSWAP_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(COMMON_OBJS) $(ENDIANSWAP_OBJS)


.PHONY: clean
clean:
	-$(RM) *~ $(COMMON_OBJS) $(THOMSON_KERNEL_OBJS) $(REDTABLE_OBJS) \
		$(V1_MOST_FLASH_OBJS) $(V2_MOST_FLASH_OBJS) $(ENDIANSWAP_OBJS) $(PROGRAMS)

