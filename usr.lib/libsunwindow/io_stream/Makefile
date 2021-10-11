#
#	@(#)Makefile	92/07/30 1.1
#
WHOAMI=		io_stream
#HDRSPUBLIC=	io_stream.h
CFILESLIB=	input.c output.c io.c \
		string_streams.c file_streams.c other_streams.c
SPCFILESLIB=	file_streams.c other_streams.c string_streams.c 
OBJS = $(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/input.c ${FULLPATH}/output.c \
		${FULLPATH}/string_streams.c ${FULLPATH}/io.c \
		${FULLPATH}/file_streams.c ${FULLPATH}/other_streams.c \
		${FULLPATH}/io_stream.h

all : xall

#include common stuff from ../Makefile.master
include ../Makefile.master

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =

xall : $$(LIBS)
$(LIBS) :$$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

file_streams.o: file_streams.c io_stream.h \
	${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h ${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h 
input.o: input.c io_stream.h \
	${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h ${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h 
io.o: io.c io_stream.h ${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h ${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h 
other_streams.o: other_streams.c io_stream.h \
	${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h ${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h 
output.o: output.c io_stream.h \
	${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h ${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h 
string_streams.o: string_streams.c io_stream.h \
	${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h ${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h 

clean: master.clean
