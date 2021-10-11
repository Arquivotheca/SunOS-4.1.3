#
#	@(#)Makefile	1.1	92/07/30 SMI
#
WHOAMI=		window
#HDRSPUBLIC=	window.h
#HDRSPRIVATE=	window_impl.h ../frame/frame.h
CFILESLIB=      window.c window_get.c window_set.c window_loop.c 
SPCFILESLIB=	window.c window_set.c
OBJS=		$(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/window.c ${FULLPATH}/window_get.c \
		${FULLPATH}/window_set.c ${FULLPATH}/window_loop.c \
		${FULLPATH}/window_impl.h ${FULLPATH}/window.h

all: xall

include ../Makefile.arch
include ../Makefile.master

.KEEP_STATE:

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =

xall:   $$(LIBS)
$(LIBS) : $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

clean: master.clean

demo.o : window.h ../frame/frame.h

demo: demo.o libsuntool.a
	${CC} ${CFLAGS} ${DBX} ${LDFLAGS} ${CPUFLAG} -o demo demo.o libsuntool.a

