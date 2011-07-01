CC 	= msp430-gcc
CFLAGS 	= -Wall -g 


OBJS 	= main.o


all: $(OBJS)
	$(CC) $(CFLAGS) -o handstandman.elf $(OBJS)

install: all
	mspdebug rf2500 "prog handstandman.elf"

%.o: %.c
	$(CC) $(CFLAGS) -c $<

erase:
	mspdebug rf2500 "erase"

clean:
	rm -fr handstandman.elf $(OBJS)


