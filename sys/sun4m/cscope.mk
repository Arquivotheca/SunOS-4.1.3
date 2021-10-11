# @(#)cscope.mk	1.1
#
# builds a cscope.out file for use by cscope -d
# uses "files files" to create the cscope.files list
#
# usage: make -f cscope.mk
# result: cscope.out
# side effect: cscope.files
# cscope usage: cscope -d
# (all this stuff takes place and lives in the $(ARCH) directory)

ARCH:sh = basename `pwd`
KERNEL=		GENERIC
FILES=	cscope.files
OUT=	cscope.out
TMPFILE=/tmp/cscope.$$$$

LIBPROMLIST=	libprom.list

HEADERS=-I$(KERNEL) -I. -I..

all:		$(OUT)

$(OUT):	$(FILES) FRC
	cscope -b -i $(FILES) -f $(OUT) 2>&1 | tee /tmp/cscope.errs 

$(FILES):	$(LIBPROMLIST) FRC
	@(TMPFILE=$(TMPFILE); \
	rm -f $(FILES) $$TMPFILE; \
	echo "+ Creating list from files files..."; \
	echo $(HEADERS) > $$TMPFILE; \
	sed	-e '/#/d' \
		-e '/^include/d' \
		-e '/not-supported/d' \
		-e '/^$$/d' \
		-e 's/[	 ].*$$//p' \
		-e 's/^/\.\.\//' \
		../conf.common/files.cmn\
		conf/files\
		>> $$TMPFILE; \
	echo "+ Adding handcrafted stuff not in files files..."; \
	echo ../conf.common/param.c >> $$TMPFILE; \
	echo genassym.c >> $$TMPFILE; \
	echo $(KERNEL)/assym.s >> $$TMPFILE; \
	echo $(KERNEL)/percpu.s >> $(TMPFILE); \
	echo $(KERNEL)/ioconf.c >> $$TMPFILE; \
	echo $(KERNEL)/confvmunix.c >> $$TMPFILE; \
	echo $(KERNEL)/vers.c >> $$TMPFILE; \
	echo "+ Adding files from $(LIBPROMLIST) ..."; \
	cat $(LIBPROMLIST) >> $$TMPFILE; \
	echo "+ Sorting and removing duplicates..."; \
	sort $$TMPFILE | uniq > $(FILES); \
	rm -f $$TMPFILE; \
	);

FRC:

