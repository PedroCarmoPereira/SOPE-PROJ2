CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -g
SRCS = $(wildcard *.c) ../utils/log.c
TARGETS = user

define make_target
$(1): $(1).o
	$(CC) $(CFLAGS) $(SRCS) -o $$@
endef

.PHONY: all clean

all: $(TARGETS)

ifdef DEV_INFO
DEV_FLAGS = -save-temps
endif

user: $(SRCS:.c=.o)
	$(CC) $(CFLAGS) $(SRCS) -o $@

%.o: %.c 
	$(CC) $(CFLAGS) $(DEV_FLAGS) -MMD -c $< -o $@

clean:
	rm -f $(TARGETS) *.o *.d *.i *.s

-include $(TARGETS:=.d)
