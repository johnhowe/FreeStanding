CC 	= msp430-gcc
CFLAGS 	= -Wall -g 


OBJS 	= main.o


all: $(OBJS)
	$(CC) $(CFLAGS) -o freestanding.elf $(OBJS) -mmcu=msp430g2231

install: all
	mspdebug rf2500 "prog freestanding.elf"

%.o: %.c
	$(CC) $(CFLAGS) -c $<

erase:
	mspdebug rf2500 "erase"

clean:
	rm -fr freestanding.elf $(OBJS)


