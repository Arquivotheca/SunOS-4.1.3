#
#  @(#)Makefile 1.3 91/08/29 SMI
#

FILE_DB_B_DIR = ${DESTDIR}/file_db.b

TEXTFILES = kvm_media_files_database.sun4 \
        kvm_media_files_database.sun4c \
        kvm_media_paths.sun4 \
        kvm_media_paths.sun4c \
        root_media_files_database \
        root_media_paths \
        usr_media_files_database \
        usr_media_paths

all:	${TEXTFILES}

install:	all
	install -d -m 755 ${FILE_DB_B_DIR}
	for i in $(TEXTFILES); do \
		install -m 755 $$i ${FILE_DB_B_DIR}/$$i; \
	done

clean:
	rm -rf ${FILE_DB_B_DIR}
