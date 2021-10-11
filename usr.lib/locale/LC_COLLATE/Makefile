#
# make file of colldef
#
#
#	@(#)Makefile 1.1 92/07/30 SMI"; 
#
COLLDIR = usr/share/lib/locale/LC_COLLATE
COLLFILE = C
YFLAGS = -d
PROG = colldef
SRC  = main.c lex.c utils.c charmap.c substitute.c order.c 
HDRS = colldef.h extern.h 
OBJS = colldef.o lex.o utils.o  main.o charmap.o substitute.o  \
	order.o 
CFLAGS =  

.KEEP_STATE:

all:	$(PROG) $(COLLFILE)

C: $(PROG) C.in
	$(PROG)  C < C.in

$(PROG): $(HDRS)	$(OBJS)
	cc $(OBJS) -lm -o $(PROG) 

lint:
	lint $(SRC)

clean:
	rm -f $(OBJS) y.tab.[ch] $(PROG) $(COLLFILE)

install: $(PROG) $(COLLFILE)
	install -s $(PROG) $(DESTDIR)/usr/etc
	install -d -m 755 $(DESTDIR)/$(COLLDIR)
	install -m 644 $(COLLFILE) $(DESTDIR)/$(COLLDIR)
	rm -f $(DESTDIR)/$(COLLDIR)/default
	ln -s ./$(COLLFILE) $(DESTDIR)/$(COLLDIR)/default

echo :
	@echo $(HDRS) colldef.y $(SRC)
