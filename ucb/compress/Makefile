#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 5.5 85/09/18
#
COMFLAGS=-DBSD4_2 -O -DSACREDMEM=256000 -DBITS=16
BIN=${DESTDIR}/usr/ucb
CC= /bin/cc

compress: compress.c
	$(CC) $(COMFLAGS) -o compress compress.c

install: compress
	install -s compress $(BIN)
	rm -f $(BIN)/uncompress $(BIN)/zcat
	ln $(BIN)/compress $(BIN)/uncompress
	ln $(BIN)/compress $(BIN)/zcat

clean:
	rm -f compress core errs
