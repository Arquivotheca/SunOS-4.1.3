#
# @(#)Makefile 1.1 92/07/30
#
all : xall
include ../MakeCommon

DIR = ..
CFLAGS  += -O -DSUN2=0 -DSUN3=1 -DSUN4=2 -Dsun3 -DDEBUG -DTARGET=$(TARGET) -I..
CPPLFAGS += -I..
LINTFLAGS = 
HDRS = dynamic.h reloc_info.h
PROGRAM = ld

xall : ${HDRS:%.h=$(DIR)/%.h} ${PROGRAM}

${PROGRAM}:	${LIBS}	${OBJS}
	${CC} -o ${PROGRAM} ${OBJS} ${LIBS}

install: ${PROGRAM}
	install -s ${PROGRAM} ${DESTDIR}/bin/${PROGRAM}

clean: masterclean

tags:
	ctags $(MID_SRCS)

lint : masterlint
