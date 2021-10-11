#
# @(#)Makefile 1.1 92/07/30 SMI
#
DESTDIR=
SYSDIR= ../sys
CFLAGS= -O 
LDFLAGS=
#CC=    /usr/bin/cc
CHOWN=/usr/etc/chown
#MAKE= make CC=${CC}
LIBC=

# System add methods that need to be installed
#
SYS_METHODS= sys_add sys_domain sys_hostname sys_ipaddr sys_time sys_tz

# System administration programs that need to be installed
#
SYS_APPS= sysIDtool sys_rootpasswd

# Programs that need to be installed
#
NSTD= ${SYS_METHODS} ${SYS_APPS}

FILES= class
# Routines to link with
#
ADMIN_LIB= admin_amcl.o admin_param.o find_method.o

# Header file dependencies
#
HDRS= admin_amcl.h admin_messages.h sys_param_names.h


all: ${NSTD} ${FILES}

install: ${NSTD} ${OTHER} ${EZNET_ETC} ${EZNET_SBIN} ${FILES}
	install -d -o bin -m 755 ${DESTDIR}/usr/etc/admin/classes/system
	install -d -m 755 ${DESTDIR}/usr/etc/install
	install -s ${SYS_METHODS} ${DESTDIR}/usr/etc/admin/classes/system
	install -s ${SYS_APPS} ${DESTDIR}/usr/etc/install
	for i in ${FILES}; do \
		(install -c -m 644 $$i ${DESTDIR}/usr/etc/admin/classes/.$$i); \
		done

clean:
	rm -f a.out core *.s *.o

sysIDtool:	sysIDtool.c ${ADMIN_LIB} get_ether0name.o ${HDRS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c ${ADMIN_LIB} get_ether0name.o

sys_add:	sys_add.c ${ADMIN_LIB} ${HDRS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c ${ADMIN_LIB}

sys_domain:	sys_domain.c ${ADMIN_LIB} ${HDRS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c ${ADMIN_LIB}

sys_hostname:	sys_hostname.c ${ADMIN_LIB} get_ether0name.o ${HDRS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c ${ADMIN_LIB} get_ether0name.o

sys_ipaddr:	sys_ipaddr.c ${ADMIN_LIB} add_key_entry.o \
		${HDRS} add_key_entry.h
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c ${ADMIN_LIB} add_key_entry.o

sys_time:	sys_time.c ${ADMIN_LIB} ${HDRS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c ${ADMIN_LIB} 

sys_tz:	sys_tz.c ${ADMIN_LIB} ${HDRS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c ${ADMIN_LIB}

sys_rootpasswd:	sys_rootpasswd.c ${ADMIN_LIB} ${HDRS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $@.c ${ADMIN_LIB}

add_key_entry.o: add_key_entry.c admin_amcl.h add_key_entry.h
	${CC} ${CFLAGS} ${LDFLAGS} -c $@ add_key_entry.c

admin_amcl.o:	admin_amcl.c admin_amcl.h admin_amcl_impl.h admin_messages.h
	${CC} ${CFLAGS} ${LDFLAGS} -c $@ admin_amcl.c

admin_param.o:	admin_param.c admin_amcl.h
	${CC} ${CFLAGS} ${LDFLAGS} -c $@ admin_param.c

find_method.o:	find_method.c find_method_impl.h admin_messages.h
	${CC} ${CFLAGS} ${LDFLAGS} -c $@ find_method.c

FRC:

