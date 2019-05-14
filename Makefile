all:
	+$(MAKE) -C server
	+$(MAKE) -C user

.PHONY: all clean

clean:
	+$(MAKE) -C server clean
	+$(MAKE) -C user clean

