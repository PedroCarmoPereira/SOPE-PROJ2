CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -g -D_REENTRAT -pthread -lrt
SRCS = $(wildcard *.c) ../utils/log.c
TARGETS = server

define make_target
$(1): $(1).o
	$(CC) $(CFLAGS) $(SRCS) -o $$@
endef

.PHONY: all clean

all: $(TARGETS)

ifdef DEV_INFO
DEV_FLAGS = -save-temps
endif

#$(foreach TARGET, $(TARGETS), $(eval $(call make_target, $(TARGET)))) #apagar dps
server: $(SRCS:.c=.o)
	$(CC) $(CFLAGS) $(SRCS) -o $@

%.o: %.c 
	$(CC) $(CFLAGS) $(DEV_FLAGS) -MMD -c $< -o $@

clean:
	rm -f $(TARGETS) *.o *.d *.i *.s;

-include $(TARGETS:=.d)
