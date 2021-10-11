#
#        @(#)Makefile  1.1     92/07/30 SMI
#
# Copyright (c) 1987 by Sun Microsystems, Inc
WHOAMI=sunwindow
GLOBAL_DATA = pw_rop_data.o win_misc_data.o notify_data.o rect_data.o

all:	xall

include Makefile.arch
include Makefile.master

VERS-mc68010 = version
VERS-mc68020 = $(VERS-mc68010)
VERS-sparc = $(VERS-mc68010)

SUBDIRS= attr misc pw rect win defaults filter io_stream notify string_utils

SOURCE_SCCS=${SCCS_DIRS}/SCCS_DIRECTORIES/usr.lib/lib${WHOAMI}
SUBTREE_ROOT=

SPECIAL_MACRO1="NO_XSTR=${NO_XSTR}"
SPECIAL_MACRO1${NO_XSTR}=

SPECIAL_MACRO2="DBX=${DBX}"
SPECIAL_MACRO2${DBX}=

xall:	$(LIBS) $(LIBS_SA)

MAKE_C_MACROS= "INCLUDE=${INCLUDE}" \
	"TARGET_ARCH=${ARCH}" "DESTDIR=${DESTDIR}" \
	"LINTFLAGS=${LINTFLAGS}"
ZMAKE= ${MAKE} ${MFLAGS} ${MAKE_C_MACROS} ${SPECIAL_MACRO1} \
	${SPECIAL_MACRO2}


XSTR= strings

$(LIBS) : $(XSTR) $(SUBDIRS) $$(MAKE_TMP) $$(DIRS) objsort lorder-sparc
	$(FINAL_COMMAND)

$(XSTR) : FRC
	@if [ ! -f strings ]; then \
		if [ -f $(LIB).strings ]; then \
			mv $(LIB).strings strings; \
		else \
			touch strings; \
		fi; \
	fi

${SUBDIRS}: FRC
	cd $@; \
	rm -f strings; \
	ln ../strings strings; \
	$(ZMAKE)  $(LIB)

libsunwindow.a 			:= DIRS = $(SUBDIRS:%=%.load_$(LIB))
libsunwindow_p.a		:= DIRS = $(SUBDIRS:%=%.load_$(LIB))
libsunwindow_g.a		:= DIRS = $(SUBDIRS:%=%.load_$(LIB))
libsunwindow.so			:= DIRS = $(SUBDIRS:%=%.load_$(LIB))

HELP = $(LIB)($(@:%.load_$(LIB)=%)/$(VARIANT)/*.o)
$(SUBDIRS:%=%.load_libsunwindow.a) $(SUBDIRS:%=%.load_libsunwindow_p.a) $(SUBDIRS:%=%.load_libsunwindow_g.a): $$(HELP)
	ar lrv $(LIB)  $?
$(SUBDIRS:%=%.load_libsunwindow.so): FRC
	ln  $(@:%.load_libsunwindow.so=%)/$(VARIANT)/*.o tmp/

libsunwindow.so libsunwindow_p.so	:= MAKE_TMP = make_tmp
libsunwindow.a libsunwindow_p.a libsunwindow_g.a	:= MAKE_TMP = FRC

make_tmp: FRC
	rm -fr tmp
	mkdir tmp

FINAL_COMMAND = @true
libsunwindow.a	libsunwindow_p.a libsuntool_g.a	:= FINAL_COMMAND = \
	-if [ "${NO_XSTR}" = "" ]; then \
	    xstr -l _libsunwindow_xstr; \
	    $(CC) $(CFLAGS_NOT_ROI_DATA) $(ROI_DATA) $(CFLAGS$(TARGET_MACH)) \
		    -o $(LIB).xs.o -c xs.c; \
	    ar lrv $(LIB) $(LIB).xs.o; \
	    rm -f xs.c; \
	    mv strings $(LIB).strings; \
	fi; \
	ranlib $@
	
libsunwindow.so libsunwindow_p.so	:= FINAL_COMMAND = \
	xstr -l _libsunwindow_xstr; \
	$(CC) $(CFLAGS_NOT_ROI_DATA) $(ROI_DATA) $(CFLAGS$(TARGET_MACH)) \
		-o tmp/$(LIB).xs.o -c xs.c; \
	$(LD) `objsort lorder-sparc tmp`; mv a.out $(LIB); \
	rm -f xs.c; \
	mv strings $(LIB).strings

libsunwindow.sa: FRC
	rm -rf tmp_sa
	mkdir tmp_sa
	cd tmp_sa; ar x ../libsunwindow.a $(GLOBAL_DATA);ar rv ../$@ \
		$(GLOBAL_DATA)
	ranlib $@

$$(LIB)(%.o): %.o
	@true


clean:  FRC
	for i in ${SUBDIRS}; do \
	(	cd $$i; \
		$(MAKE) $(MFLAGS) $(LINT_FLAGS) $(LIB); \
	); done
	rm -rf tmp libsunwindow.a libsunwindow_p.a libsunwindow_g.a libsunwindow.so \
	libsunwindow_p.so libsunwindow.sa strings libsunwindow.a.strings \
	libsunwindow_p.a.strings libsunwindow.so.strings \
	libsunwindow_p.so.strings libsunwindow.sa.strings \
	libsunwindow.a.xs.o libsunwindow_p.a.xs.o libsunwindow.so.xs.o \
	libsunwindow_p.so.xs.o libsunwindow.sa.xs.o tags

clean_obj clean_obj_p clean_obj_g clean_shared: $(SUBDIRS)

# copy and strip when install
install:: $(VERS$(TARGET_MACH))  install_libsunwindow.sh install_lint
	-install -c -m 644 libsunwindow.a ${DESTDIR}/usr/lib
	${RANLIB} ${DESTDIR}/usr/lib/libsunwindow.a
	-install -c -m 644 libsunwindow_p.a ${DESTDIR}/usr/lib
	${RANLIB} ${DESTDIR}/usr/lib/libsunwindow_p.a
	sh -x install_libsunwindow.sh $(INSTALL) $(VERS$(TARGET_MACH)) $(DESTDIR)

llib-lsunwindow.c:
	@if [ ! -f $@ ]; then \
		sccs get $@; \
	fi
llib-lsunwindow.ln: llib-lsunwindow.c
	${LINT} -I${INCLUDE} -Csunwindow llib-lsunwindow.c

install_lint:  llib-lsunwindow.ln
	-install -c -m 644 llib-lsunwindow.c ${DESTDIR}/usr/lib/lint/llib-lsunwindow
	-install -c -m 644 llib-lsunwindow.ln ${DESTDIR}/usr/lib/lint/llib-lsunwindow.ln

install_h:: FRC
	-@if [ ! -d ${PRODINCLUDEDIR} ]; then \
		mkdir ${PRODINCLUDEDIR} && \
		chown bin ${PRODINCLUDEDIR} && \
		chmod 755 ${PRODINCLUDEDIR}; \
	fi
	-@for i in include/sunwindow; do (cd $$i; \
		make ${MFLAGS} DESTDIR=${DESTDIR} install_subdir_h); done

new_subdir:     create_subdir usr_latest
 
create_subdir:
	-@for subdir in ${SUBDIRS}; do \
	(   if [ ! -d $${subdir} ]; then \
		echo "Adding new subdirectory $${subdir} ..."; \
		mkdir $${subdir}; \
		cd $${subdir}; \
			ln -s ${SOURCE_SCCS}/$${subdir}/SCCS .; \
			ln -s ../tags .; \
	   fi; \
	) done
 
subtree:
	-@if [ "${SUBTREE_ROOT}" = "" ]; then \
	    echo -n "SUBTREE_ROOT not defined."; \
	    if [ -d attr ]; then \
		echo "  Updating current hierarchy's subtree."; \
		${ZMAKE} -s "SUBDIRS=${SUBDIRS}" new_subdir; \
	    else \
		echo "  This does not look like a subtree-root, aborting."; \
	    fi; \
	elif [ -d ${SUBTREE_ROOT} ]; then \
	    cd ${SUBTREE_ROOT}; \
	    echo "Making directories"; \
	    mkdir -p usr/src/usr.lib/lib${WHOAMI}; \
	    mkdir -p ${SUBTREE_ROOT}/usr_latest/include/${WHOAMI}; \
	    cd usr/src/usr.lib/lib${WHOAMI}; \
	    rm -f SCCS;\
	    ln -s ${SOURCE_SCCS}/SCCS .; \
		for subdir in ${SUBDIRS}; do \
		( \
		    echo "Directory $${subdir}"; \
		    mkdir $${subdir}; \
		    cd $${subdir}; \
		    rm -f SCCS tags; \
		    echo "Making SCCS links"; \
		    ln -s ${SOURCE_SCCS}/$${subdir}/SCCS .; \
		    ln -s ../tags .; \
		    echo "Making include files links"; \
		    ${MAKE} "DESTDIR=${SUBTREE_ROOT}" -s usr_latest; \
		    cd ..; \
		); done; \
	else \
	     echo "${SUBTREE_ROOT}: No such file or directory"; \
	fi
usr_latest::
	-mkdir ${DESTDIR}/usr_latest ${DESTDIR}/usr_latest/include ${DESTDIR}/usr_latest/include/sunwindow
	-for i in ${SUBDIRS}; do \
	(       echo "Descending into directory: $$i"; \
		cd $$i; make ${MFLAGS} "INCLUDE=${INCLUDE}"\
		"CFLAGS=${CFLAGS}" "DBX=${DBX}" "CPUFLAG=${CPUFLAG}"\
		"CC=${CC}" "DESTDIR=${DESTDIR}" "MAKE=${MAKE}"\
		"LINTLIBS=${LINTLIBS}" usr_latest \
	); done

hdr src::
	set +e; \
	for i in ${SUBDIRS}; do \
		(cd $$i && $(MAKE) $(MFLAGS) $@); \
	done

tags::
	touch tags
	-for i in ${SUBDIRS}; do (cd $$i; make ${MFLAGS} "DESTDIR=${DESTDIR}" tags); done

lint::
	-for i in ${SUBDIRS}; do \
	(       echo "Descending into directory: $$i"; \
		cd $$i; \
		${MAKE} ${MFLAGS} "DESTDIR=${DESTDIR}" "LINTFLAGS=${LINTFLAGS}" lint \
	); done

