#
# @(#)Makefile 1.1 92/07/30 SMI
#

CFILESALL=	align_equals.c capitalize.c \
		insert_brackets.c remove_brackets.c \
		bracket_misc.c shift_lines.c
OBJFILES=	bracket_misc.o
HDRSPRIVATE=
HDRSPUBLIC=
HDRSALL= ${HDRSPRIVATE} ${HDRSPUBLIC}
SRCSALL=$(CFILESALL) $(HDRSALL) Makefile
GPROFFILES= align_equals.pg capitalize.pg insert_brackets.pg \
	    remove_brackets.pg bracket_misc.pg shift_lines.pg

BINS=	align_equals capitalize insert_brackets \
	remove_brackets shift_lines

# CMDS are all of the programs made in this directory
CMDS=	${BINS}

.KEEP_STATE:

all: ${HDRSALL} ${OBJFILES} ${CMDS}

.c.o:
	${CC} ${CFLAGS_NOT_ROI_DATA} -c $<

include ../../Makefile.master

objsall: ${OBJFILES}

gprof:: ${GPROFFILES}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof

lint:: ${CFILESALL}
	${MAKE} ${MFLAGS} ${COMMONFLAGS} lint

insert_brackets: insert_brackets.c bracket_misc.o     
	${CC} -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} -o insert_brackets insert_brackets.c bracket_misc.o

remove_brackets: remove_brackets.c bracket_misc.o     
	${CC} -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} -o remove_brackets remove_brackets.c bracket_misc.o 

align_equals: align_equals.c
	${CC} -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} -o align_equals align_equals.c $(LIBS)

capitalize: capitalize.c    
	${CC} -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} -o capitalize capitalize.c   

shift_lines: shift_lines.c
	${CC} -DSTANDALONE ${CFLAGS_NOT_ROI_DATA} -o shift_lines shift_lines.c $(LIBS)  

FRC:
# non-existent so always younger

install: install_bins 

install_bins:
	   -@for i in ${BINS}; do \
		echo "installing $$i"; \
		if [ -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i ]; then \
		   rm -f ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		fi; \
		install -m 755 $$i ${DESTDIR}${PRODBINDIR}${LINKDIR}/$$i; \
		ln -s ${LINKDIR}/$$i ${DESTDIR}${PRODBINDIR}$$i; \
	   done

clean:
	rm -f *.o core $(CMDS) 

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);

print:	$(SRCSALL)
	lpr -p -Pimagen $(SRCSALL)
