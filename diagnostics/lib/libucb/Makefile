#
#	@(#)Makefile	1.1 7/30/92 Copyright Sun Microsystems Inc.
#

OS=SVR4
#specify "OS=" for 4.x build

SRC	= ucblib.c
OBJ	= $(SRC:.c=.o)

UCBLIB	= libucb.a

CFLAGS	= -D$(OS)

.KEEP_STATE:

all: $(UCBLIB)


$(UCBLIB): $(OBJ)
	ar cru $(UCBLIB) $(OBJ)

clean: FRC
	rm -f $(UCBLIB) $(OBJ)

info: FRC
	sccs info

FRC:
