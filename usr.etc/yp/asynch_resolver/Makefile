#
# @(#)Makefile 1.1 92/07/30 (C) 1989 SMI
#
CFLAGS=-O
LOBJS=  nres_rcv.o nres_search.o nres_send.o  herror.o nget_answer.o ngethostbyname.o rpc_as.o svc_run_as.o svc_udp.o
LINTSRCS=  nres_rcv.c nres_search.c nres_send.c  herror.c nget_answer.c ngethostbyname.c rpc_as.c svc_run_as.c 
OBJS= main.o ${LOBJS}
all:	nres.h rpc_as.h libnres.a
libnres.a:	${LOBJS}
	rm -f libnres.a
	ar cr libnres.a ${LOBJS}
	ranlib libnres.a
lint:
	lint	 $(LINTSRCS)
$(LINTSRCS):	nres.h rpc_as.h

main:	${OBJS} nres.h
	cc -o main -g main.o libnres.a -lresolv
$(LOBJS):	nres.h

install:	
	
clean:
	rm	-f ${LOBJS} libnres.a

