TARGETMCU       ?= msp430x2231

CROSS           := msp430-
CC              := $(CROSS)gcc
CXX             := $(CROSS)g++
OBJDUMP         := $(CROSS)objdump
SIZE            := $(CROSS)size
LD              := $(CC)
MSPDEBUG        := mspdebug
LDFLAGS         := -mmcu=$(TARGETMCU)
CFLAGS          := -Os -Wall -W -Wextra -Werror -g -mmcu=$(TARGETMCU)

ifneq ($(WITH_CXX),)
	CC      := $(CXX)
	LD      := $(CC)
endif

ifneq ($(DEBUG),)
	ifeq ($(WITH_CXX),)
	CFLAGS  += -Wstrict-prototypes -Wmissing-prototypes -Wbad-function-cast
	CFLAGS  += -Werror-implicit-function-declaration -Wdeclaration-after-statement
	CFLAGS  += -Wnested-externs -Wold-style-definition
	CFLAGS  += -finline-functions
endif
CFLAGS  += -Wmissing-declarations -Winit-self -Winline -Wredundant-decls
CFLAGS  += -Wshadow -Wpointer-arith -Wcast-qual -Wsign-compare -Wformat=2
CFLAGS  += -Wfloat-equal -Wmissing-field-initializers
CFLAGS  += -Wmissing-include-dirs -Wswitch-default -Wpacked
CFLAGS  += -Wpacked -Wlarger-than-65500 -Wunknown-pragmas
CFLAGS  += -Wmissing-format-attribute -Wmissing-noreturn
CFLAGS  += -Wstrict-aliasing=2 -Wswitch-enum -Wundef -Wunreachable-code
CFLAGS  += -Wunsafe-loop-optimizations -Wwrite-strings -Wcast-align
CFLAGS  += -Wformat-nonliteral -Wformat-security -Waggregate-return

CFLAGS  += -fno-common -Wp,-D_FORTIFY_SOURCE=2
endif

SRCS            := main.c
PROG            := $(firstword $(SRCS:.c=))
OBJS            := $(SRCS:.c=.o)

all:            $(PROG).elf $(PROG).lst

$(PROG).elf:    $(OBJS)
	$(LD) $(LDFLAGS) -o $(PROG).elf $(OBJS)

%.o:            %.c
	$(CC) $(CFLAGS) -c $<

%.lst:          %.elf
	$(OBJDUMP) -DS $< > $@
	$(SIZE) $<

clean:
	-rm -f $(PROG).elf $(PROG).lst $(OBJS)

install:        $(PROG).elf
	$(MSPDEBUG) -n rf2500 "prog $(PROG).elf"
