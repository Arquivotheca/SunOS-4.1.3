#
# @(#)Makefile 1.1 92/07/30 SMI; from UCB 4.2 06/12/83
#
BINS=	in.ftpd
OBJS=	ftpd.o ftpcmd.o glob.o popen.o logwtmp.o vers.o
SRCS=	ftpd.c ftpcmd.y ../../ucb/ftp/glob.c popen.c logwtmp.c vers.c

CFLAGS=	-O

.KEEP_STATE:

all: $(BINS)

$(BINS) : $(OBJS)
	$(LINK.c) -o $@ $(OBJS)

glob.o : ../../ucb/ftp/glob.c
	$(COMPILE.c) ../../ucb/ftp/glob.c

install: $(BINS)
	install -d $(DESTDIR)/usr/etc
	install -s $(BINS) $(DESTDIR)/usr/etc

clean:
	$(RM) $(BINS) $(OBJS)

tags:	$(SRCS)
	ctags $(SRCS)
