#
#  @(#)Makefile 1.1 92/07/30 SMI
#

FILE_DB_DIR = ${DESTDIR}/file_db.412

TEXTFILES = kvm_media_files_database.sun4 \
        kvm_media_files_database.sun4c \
        kvm_media_files_database.sun4m \
        kvm_media_paths.sun4 \
        kvm_media_paths.sun4c \
        kvm_media_paths.sun4m \
        root_media_files_database \
        root_media_paths \
        usr_media_files_database \
        usr_media_paths

all:	${TEXTFILES}

install:	all
	install -d -m 755 ${FILE_DB_DIR}
	for i in $(TEXTFILES); do \
		install -m 755 $$i ${FILE_DB_DIR}/$$i; \
	done

clean:
	rm -rf ${FILE_DB_DIR}
