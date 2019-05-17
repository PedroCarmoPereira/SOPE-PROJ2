all:
	+$(MAKE) -C srcServer
	mv srcServer/server .;
	+$(MAKE) -C srcUser
	mv srcUser/user .;

.PHONY: all clean

clean:
	+$(MAKE) -C srcServer clean
	+$(MAKE) -C srcUser clean
	rm server;
	rm user;
	rm ./utils/*.o ./utils/*.d
