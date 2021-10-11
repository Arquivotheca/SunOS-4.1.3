#
#	@(#)Makefile	92/07/30 1.1 SMI
#
WHOAMI=		defaults
#HDRSPUBLIC=	defaults.h
#HDRSPRIVATE=	defaults_impl.h hash.h parse.h
CFILESLIB=	defaults.c defaults_put.c parse.c hash.c
SPCFILESLIB=	defaults.c
SPCOPTIMIZER=	defaults.c
OBJS = $(CFILESLIB:%.c=$(VARIANT)/%.o)
TAGFILESALL=	${FULLPATH}/defaults.c ${FULLPATH}/defaults_put.c \
		${FULLPATH}/parse.c \
		${FULLPATH}/hash.c ${FULLPATH}/defaults.h \
		${FULLPATH}/hash.h ${FULLPATH}/parse.h

all : xall

#include common stuff from ../Makefile.master
include ../Makefile.master

xall : $$(LIBS)
$(LIBS): $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =
$(SPCOPTIMIZER:%.c=obj/%.o) \
	$(SPCOPTIMIZER:%.c=obj_p/%.o) \
	$(SPCFILESLIB:%.c=shared/%.o) := DBX = -O1

hash_test:	hash_test.o hash.o parse.o
	${CC} $(DBX) hash_test.o hash.o parse.o -o hash_test

defaults_test:	defaults_test.o ${LIBFILE}
	${CC} $(DBX) defaults_test.o ${LIBFILE} -o defaults_test

defaults_put.o: defaults_put.c /usr/include/stdio.h /usr/include/pwd.h \
	${INCLUDE}/sys/types.h \
	${INCLUDE}/sys/sysmacros.h \
	${INCLUDE}/sys/dir.h \
	${INCLUDE}/sys/stat.h \
	${INCLUDE}/sunwindow/sun.h defaults_impl.h hash.h parse.h \
	/usr/include/strings.h \
	${INCLUDE}/sunwindow/defaults.h	
defaults_test.o: defaults_test.c \
	${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h \
	${INCLUDE}/sunwindow/defaults.h 
hash.o: hash.c ${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h hash.h 
hash_test.o: hash_test.c /usr/include/stdio.h \
	${INCLUDE}/sunwindow/sun.h hash.h 
parse.o: parse.c ${INCLUDE}/sunwindow/sun.h \
	/usr/include/stdio.h parse.h 

clean: master.clean
