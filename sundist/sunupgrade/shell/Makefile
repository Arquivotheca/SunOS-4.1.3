#
#  @(#)Makefile 1.3 91/08/29 SMI
#

SHELL_DIR = ${DESTDIR}/shell
SHELLSC = check_perm sunupgrade
TEXTFILES = check_test exec_upgrade formatter house_keeper inout script_gen\
		check_install upgrade upgrade_client


all:	${TEXTFILES} ${SHELLSC}

${TEXTFILES}: 

${SHELLSC}:
	rm -f $@
	cp -p $@.sh $@
	chmod +x $@

install:	all
	install -d -m 755 ${SHELL_DIR}
	for i in $(TEXTFILES); do \
		install -m 755 $$i ${SHELL_DIR}/$$i; \
	done
	for i in ${SHELLSC}; do \
		install -m 755 $$i ${SHELL_DIR}/$$i; \
	done

clean:
	rm -f ${SHELLSC}
