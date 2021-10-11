# 
#        @(#)Makefile  1.1     92/07/30 SMI
#
WHOAMI=suntool
GLOBAL_DATA = tool_commons.o panel_util_data ttysw_data.o \
	      misc_data.o scrollbar_data. sel_writable_da \
	      wmgr_data.o walkmenu_data.o
all:	xall
include Makefile.arch
include Makefile.master

SUBDIRS=  tool frame canvas gfxsw scrollbar wmgr misc sel icon \
	  text ttysw menu panel alert window help 

SOURCE_SCCS=${SCCS_DIRS}/SCCS_DIRECTORIES/usr.lib/lib${WHOAMI}
SUBTREE_ROOT=

VERS-mc68010 = version
VERS-mc68020 = $(VERS-mc68010)
VERS-sparc = $(VERS-mc68010)

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

libsuntool.a			:= DIRS = $(SUBDIRS:%=%.load_$(LIB))
libsuntool_p.a 			:= DIRS = $(SUBDIRS:%=%.load_$(LIB))
libsuntool_g.a 			:= DIRS = $(SUBDIRS:%=%.load_$(LIB))
libsuntool.so			:= DIRS = $(SUBDIRS:%=%.load_$(LIB))

HELP = $(LIB)($(@:%.load_$(LIB)=%)/$(VARIANT)/*.o)
$(SUBDIRS:%=%.load_libsuntool.a) $(SUBDIRS:%=%.load_libsuntool_g.a) $(SUBDIRS:%=%.load_libsuntool_p.a) : $$(HELP)
	ar lrv $(LIB)  $?
$(SUBDIRS:%=%.load_libsuntool.so): FRC
	ln  $(@:%.load_libsuntool.so=%)/$(VARIANT)/*.o tmp/

libsuntool.so libsuntool_p.so	:= MAKE_TMP = make_tmp
libsuntool.a libsuntool_p.a libsuntool_g.a	:= MAKE_TMP = FRC

make_tmp: FRC
	rm -fr tmp
	mkdir tmp

libsuntool.a libsuntool_p.a libsuntool_g.a	:= FINAL_COMMAND = \
	-if [ "${NO_XSTR}" = "" ]; then \
	    xstr -l _libsuntool_xstr; \
	    $(CC) $(CFLAGS) $(ROI_DATA) $(CFLAGS$(TARGET_MACH)) \
		    -o $(LIB).xs.o -c xs.c; \
	    ar lrv $(LIB) $(LIB).xs.o; \
	    rm -f xs.c; \
	    mv strings $(LIB).strings; \
	fi; \
	ranlib $@
	
libsuntool.so libsuntool_p.so	:= FINAL_COMMAND = \
	xstr -l _libsuntool_xstr; \
	$(CC) $(CFLAGS) $(ROI_DATA) $(CFLAGS$(TARGET_MACH)) \
		-o tmp/$(LIB).xs.o -c xs.c; \
	$(LD) `objsort lorder-sparc tmp`; mv a.out $(LIB); \
	rm -f xs.c; \
	mv strings $(LIB).strings

libsuntool.sa: FRC
	rm -rf tmp_sa
	mkdir tmp_sa
	cd tmp_sa; ar x ../libsuntool.a $(GLOBAL_DATA);ar rv ../$@ \
		$(GLOBAL_DATA)
	ranlib $@;
	-cd tmp_sa; rm $(GLOBAL_DATA)

$$(LIB)(%.o): %.o
	@true

clean:  FRC
	for i in ${SUBDIRS}; do \
	(	cd $$i; \
		echo "<<<  Descending into directory: $$i  >>>"; \
		$(MAKE) $(MFLAGS) $(LINT_FLAGS) $(LIB); \
	); done
	rm -rf tmp libsuntool.a libsuntool_p.a libsuntool_g.a libsuntool.so \
	libsuntool_p.so libsuntool.sa strings libsuntool.a.strings \
	libsuntool_p.a.strings libsuntool.so.strings \
	libsuntool_p.so.strings libsuntool.sa.strings \
	libsuntool.a.xs.o libsuntool_p.a.xs.o libsuntool.so.xs.o \
	libsuntool_p.so.xs.o libsuntool.sa.xs.o

clean_obj clean_obj_p clean_obj_g clean_shared: $(SUBDIRS)

install:: $(VERS$(TARGET_MACH)) install_libsuntool.sh install_lint
	-install -c -m 644 libsuntool.a ${DESTDIR}/usr/lib
	${RANLIB} ${DESTDIR}/usr/lib/libsuntool.a
	-install -c -m 644 libsuntool_p.a ${DESTDIR}/usr/lib
	${RANLIB} ${DESTDIR}/usr/lib/libsuntool_p.a
	sh -x install_libsuntool.sh $(INSTALL) $(VERS$(TARGET_MACH)) $(DESTDIR)

llib-lsuntool.c:
	@if [ ! -f $@ ]; then \
		sccs get $@; \
	fi
llib-lsuntool.ln: llib-lsuntool.c
	-${LINT} -I${INCLUDE} -Iinclude -Csuntool llib-lsuntool.c

install_lint:  llib-lsuntool.ln
	-install -c -m 644 llib-lsuntool.c ${DESTDIR}/usr/lib/lint/llib-lsuntool
	-install -c -m 644 llib-lsuntool.ln ${DESTDIR}/usr/lib/lint/llib-lsuntool.ln

install_h:: FRC
	-@if [ ! -d ${PRODINCLUDEDIR} ]; then \
		mkdir ${PRODINCLUDEDIR} && \
		chown bin ${PRODINCLUDEDIR} && \
		chmod 755 ${PRODINCLUDEDIR}; \
	fi
	-@for i in include/suntool; do (cd $$i; \
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
	    if [ -d alert ]; then \
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
		    ${MAKE} "DESTDIR=${SUBTREE_ROOT}" -s usr_latest; \
		    cd ..; \
		); done; \
	else \
	     echo "${SUBTREE_ROOT}: No such file or directory"; \
	fi

usr_latest::
	-mkdir ${DESTDIR}/usr_latest ${DESTDIR}/usr_latest/include ${DESTDIR}/usr_latest/include/suntool
	-for i in ${SUBDIRS}; do \
	(       echo "Descending into directory: $$i"; \
		cd $$i; make ${MFLAGS} "INCLUDE=${INCLUDE}"\
		"CFLAGS=${CFLAGS}" "DBX=${DBX}" "CPUFLAG=${CPUFLAG}"\
		"CC=${CC}" "DESTDIR=${DESTDIR}" "MAKE=${MAKE}"\
		"LINTLIBS=${LINTLIBS}" usr_latest \
	); done
tag::
	-for i in ${SUBDIRS}; do (cd $$i; make ${MFLAGS} "DESTDIR=${DESTDIR}" tag); done

lint::
	-for i in ${SUBDIRS}; do \
	(       echo "Descending into directory: $$i"; \
		cd $$i; \
		${MAKE} ${MFLAGS} "LINTFLAGS=${LINTFLAGS}" lint \
	); done
