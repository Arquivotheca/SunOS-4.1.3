#
#
# @(#)Makefile 1.1 92/07/30 SMI
#

WHOAMI=sunview
SUBDIRS_SUNVIEW= clockperf defaults demos desktop examples fileview\
		  fontedit gfx iconedit lib mailtool extras tekfonts \
		  tektool term textedit traffic

SUBDIRS= utilities images

all: binsall

#
# All subtree related variables must be before "include Makefile.master",
# and the rules after the include.
#
SOURCE_SCCS= ${BASEDIR}/SCCS_DIRECTORIES/usr.bin/sunwindows
SUBTREE_ROOT=
SUBTREE_REL_ROOT=usr/src/usr.bin/sunwindows
SUBTREE_PATH=
SUBTREE_BACK_PATH=
SUBTREE_REL_PATH=${SUBTREE_REL_ROOT}/${SUBTREE_PATH}
SUBTREE_NODE=${SUBTREE_ROOT}/${SUBTREE_REL_PATH}

include Makefile.master

gprof:: 

all_images: 
	cd images ; $(MAKE) 
	touch all_images

binsall:
	for i in ${SUBDIRS_SUNVIEW}; do \
	(       echo "<<< Descending into directory: ${WHOAMI}/$$i >>>"; \
		cd ${WHOAMI}/$$i; \
		${MAKE} ${MFLAGS} ${COMMONFLAGS} all \
	); done
	for i in ${SUBDIRS}; do \
	(	echo "<<< Descending into directory: $$i >>>"; \
		cd $$i; \
		${MAKE} ${MFLAGS} ${COMMONFLAGS} all \
	); done
	
FRC:

ensure_subtree_root:
# Do not add - in front of the rules that can "exit 1."
	@if [ "${SUBTREE_ROOT}" = "" ]; then \
	  echo "SUBTREE_ROOT not defined."; \
	  exit 1; \
	fi
	@if [ ! -d ${SUBTREE_ROOT}/usr/src ]; then \
	  echo "SUBTREE_ROOT malformed: missing usr/src."; \
	  exit 1; \
	fi
	-@cd ${SUBTREE_ROOT}/usr/src; \
	  mkdir -p usr.bin/sunwindows
	@if [ ! -d ${SUBTREE_NODE} ]; then \
	  echo "Cannot find nor make ${SUBTREE_NODE}."; \
	  exit 1; \
	fi
	@cd ${SUBTREE_NODE}; \
	  rm -f SCCS; ln -s ${SOURCE_SCCS}/${SUBTREE_PATH}/SCCS .
#	  for i in include lib${WHOAMI}.a lib${WHOAMI}.sa lib${WHOAMI}.so; do \
#	    rm -f $$i; ln -s ../../usr.lib/lib${WHOAMI}/$$i .; \
#	  done;

subtree: ensure_subtree_root subnode

subnode:
# Do not add - in front of the rules that can "exit 1."
# Overview:
# (1) check target node exists,
# (2) cd to target and link up tags file [caller linked up SCCS],
# (3) for each subdir make a directory in the target and link up its SCCS
# (4) recursively make in source
	@if [ ! -d ${SUBTREE_NODE} ]; then \
	  echo "Cannot find nor make ${SUBTREE_NODE}."; \
	  exit 1; \
	fi
	@cd ${SUBTREE_NODE}; \
	  if [ "${SUBTREE_BACK_PATH}" != "" ]; then \
	    rm -f tags; ln -s ${SUBTREE_BACK_PATH}/tags .; \
	  fi; \
	  for subdir in ${SUBDIRS}; do \
	    ( test -d $${subdir} || mkdir $${subdir}; \
	      cd $${subdir}; \
		echo "Creating " `pwd`; \
		rm -f SCCS; \
		ln -s ${SOURCE_SCCS}/${SUBTREE_PATH}/$${subdir}/SCCS .; \
	      cd ..; \
	    ); done
	@cd ${SUBTREE_NODE}; \
	test -d ${WHOAMI} || mkdir ${WHOAMI}; \
	for subdir in ${SUBDIRS_SUNVIEW}; do \
	  ( test -d ${WHOAMI}/$${subdir} || mkdir ${WHOAMI}/$${subdir}; \
	    cd ${WHOAMI}/$${subdir}; \
	      echo "Creating " `pwd`; \
	      rm -f SCCS; \
	      ln -s ${SOURCE_SCCS}/${SUBTREE_PATH}${WHOAMI}/$${subdir}/SCCS .; \
	    cd ..; \
	  ); done; \
	cd ${WHOAMI}/defaults; \
 	test -d lib || mkdir lib; \
		cd lib; \
		echo "Creating " `pwd`; \
		rm -f SCCS; \
		ln -s ${SOURCE_SCCS}/${SUBTREE_PATH}${WHOAMI}/defaults/lib/SCCS .; \
		cd ..; \
	cd ../..

fast_install:
	-for i in ${SUBDIRS_SUNVIEW}; do \
		(cd ${WHOAMI}/$$i; ${MAKE} ${MFLAGS} ${COMMONFLAGS} fast_install); done
	-for i in ${SUBDIRS}; do \
		(cd $$i; ${MAKE} ${MFLAGS} ${COMMONFLAGS} fast_install); done

install: install_dirs Makefile.customer
	install -m 644 Makefile.customer ${DESTDIR}${SRCDIR}/Makefile
	-for i in ${SUBDIRS_SUNVIEW}; do \
		(cd ${WHOAMI}/$$i; ${MAKE} ${MFLAGS} ${COMMONFLAGS} install); done
	-for i in ${SUBDIRS}; do \
		(cd $$i; ${MAKE} ${MFLAGS} ${COMMONFLAGS} install); done

# 	Note that when creating the directories, if you are not 
# 	  running as superuser, the ${CHOWN} will not succeed.
install_dirs:
	-for i in /usr/share/src /usr/share/src/sun \
		/usr/share/src/sun/${WHOAMI}1 \
		/usr/bin/${WHOAMI}1 /usr/include/images \
		/usr/demo /usr/demo/${WHOAMI}1 \
		/usr/old; do \
			install -d -o bin -m 755 ${DESTDIR}$$i; \
	done
	-for i in /fonts /fonts/tekfonts; do \
		install -d -o bin -m 755 ${DESTDIR}/usr/lib$$i; \
	done

install_bins: 
	-for i in ${SUBDIRS_SUNVIEW}; do \
		cd ${WHOAMI}/$$i; \
		${MAKE} ${MFLAGS} DESTDIR=${DESTDIR} install_bins; \
	done
	-for i in ${SUBDIRS}; do \
		cd $$i; \
		$(MAKE) ${MFLAGS}  DESTDIR=${DESTDIR} install_bins; \
	done

install_libs:
	-for i in ${SUBDIRS_SUNVIEW}; do \
		cd ${WHOAMI}/$$i; \
		${MAKE} ${MFLAGS} DESTDIR=${DESTDIR} install_libs; \
	done
	-for i in ${SUBDIRS}; do \
		cd $$i; \
		$(MAKE) ${MFLAGS}  DESTDIR=${DESTDIR} install_libs; \
	done

install_demos:
	-for i in ${WHOAMI}/demos; do \
		cd ${WHOAMI}/$$i; \
		$(MAKE) ${MFLAGS}  DESTDIR=${DESTDIR} install_demos; \
	done

install_sources: 
	-for i in ${SUBDIRS_SUNVIEW}; do \
		-cd ${WHOAMI}/$$i; \
		$(MAKE) ${MFLAGS}  DESTDIR=${DESTDIR} install_sources; \
	done
	-for i in ${SUBDIRS}; do \
		cd $$i; \
		$(MAKE) ${MFLAGS}  DESTDIR=${DESTDIR} install_sources; \
	done

install_images:
	(cd images; $(MAKE) ${MFLAGS}  DESTDIR=${DESTDIR} install)

install_tekfonts:
	-cd ${WHOAMI}/tekfonts; \
	$(MAKE) ${MFLAGS}  DESTDIR=${DESTDIR} install

install_keymaps:
	(cd keymaps; $(MAKE) ${MFLAGS}  DESTDIR=${DESTDIR} install_keymaps)

clean:
#  for now don't clean out *.o in the top directory because of
#  setlocale_.o and ctype_.o
#  Janice Hsia 6/10/88
#
#  start cleaning all *.o
#  Janice C. Hsia 11/29/88
	-rm -f *.a *.o core *.pg *p err*
	-for i in ${SUBDIRS_SUNVIEW}; do \
	(	cd ${WHOAMI}/$$i; \
		$(MAKE) ${MFLAGS} clean; \
	); done
	-for i in ${SUBDIRS}; do (cd $$i; $(MAKE) ${MFLAGS} clean); done

tags:	$(CFILESALL)
	${CTAGS} $(CFILESALL);

print:	$(SRCSALL)
	lpr -p -Pimagen $(SRCSALL)

lint::
	-for i in ${SUBDIRS_SUNVIEW}; do \
	(       echo "Descending into directory: ${WHOAMI}/$$i"; \
		cd ${WHOAMI}/$$i; \
		${MAKE} ${MFLAGS} ${COMMONFLAGS} lint \
	); done
	-for i in ${SUBDIRS}; do \
	(	echo "Descending into directory: $$i"; \
		cd $$i; \
		${MAKE} ${MFLAGS} ${COMMONFLAGS} lint \
	); done

# gprof::
# 	-for i in ${SUBDIRS_SUNVIEW}; do \
#	(       echo "Descending into directory: ${WHOAMI}/$$i"; \
#		cd ${WHOAMI}/$$i; \
#		${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof \
#	); done
#	-for i in ${SUBDIRS}; do \
#	(	echo "Descending into directory: $$i"; \
#		cd $$i; \
#		${MAKE} ${MFLAGS} ${COMMONFLAGS} gprof \
#	); done

