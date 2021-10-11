#
# @(#)Makefile.com 1.1 92/07/30
#
# Standalone Library common makefile
#
# This file is included by ../$(ARCH)/Makefile and the target libraries
# are built in ../$(ARCH)
# 
# Certain symbols must be defined before this Makefile may be included:
#
#	ARCH		The target architecture (eg, "sun4")
#	ARCHOBJ		Architecture-specific objects in ../$(ARCH)
#	ARCHOPTS	Architecture-specific compile options
#	ARCHINS		Architecture-specific install targets
#	ARCHCLEAN	Architecture-specific clean targets
#	CONFOBJ		Common devices as listed in ../$(ARCH)/conf.c

HDEST=	$(DESTDIR)/usr/include/stand
LIBSA=	libsa.a
LIBXX=	libxx.a
LIBKADB=libsadb.a
LIBS=	$(LIBSA) $(LIBXX) $(LIBKADB)
BOOTDIR=../..
SYSDIR=	../../..
COPTS=	-O
CPPDEFS= -D$(ARCH) $(ARCHOPTS)
CPPINCS= -I.. -I$(BOOTDIR) -I$(SYSDIR)/$(ARCH) -I$(SYSDIR)
CPPBOOT= -DSTANDALONE
CPPOPTS= $(CPPDEFS) $(CPPINCS)
CFLAGS=	$(COPTS) $(CPPOPTS) $(CPPBOOT)

LINTOBJ= ../llib-lsa.ln
LDIR=	/usr/lib/lint
LINT1=	$(LDIR)/lint1
LCOPTS=	-C -Dlint $(CPPOPTS)
LOPTS=	-hxbn
LTAIL=	egrep -v 'genassym\.c' | \
	egrep -v 'struct/union .* never defined' | \
	egrep -v 'possible pointer alignment problem' ; true

$(LIBSA) :=	LIB = $(LIBSA)
$(LIBXX) :=	LIB = $(LIBXX)
$(LIBKADB) :=	LIB = $(LIBKADB)
$(LIBPROM) :=	LIB = $(LIBPROM)

.PRECIOUS: $$(LIB)

# Header files that need to be present (and installed in /usr/include/stand)
HSRC=	ardef.h param.h saio.h sainet.h scsi.h sdreg.h streg.h smreg.h \
	screg.h sireg.h sereg.h swreg.h  xderr.h

HDIR=	../stand
HDRS=	$(HSRC:%.h=$(HDIR)/%.h)
.INIT:	$(HDRS)

# Common object files
CMNOBJ=	chklabel.o common.o devio.o get.o idprom.o inet.o \
	spinning.o sprintf.o sprintn.o standalloc.o sys.o

CMNDIR=	../common
CMNSRC=	$(CMNOBJ:%.o=$(CMNDIR)/%.c)
CONFDIR= ../common
CONFSRC=$(CONFOBJ:%.o=$(CONFDIR)/%.c)

# When building library for boot blocks, define BOOTBLOCK
$(LIBXX) :=	CPPDEFS += -DBOOTBLOCK
# When building library for kadb, define KADB
$(LIBKADB) :=	CPPDEFS += -DKADB

# The library also includes files in ../$(ARCH) and ../$(MACH)
OBJ=	$$(ARCHOBJ) $(MACHOBJ) $$(CMNOBJ) $$(CONFOBJ)

$(LIBS): $$@($(OBJ))
	ranlib $@

# Rules for common .c files
$$(LIB)(%.o): $(CMNDIR)/%.c
	$(CC) $(CFLAGS) -c $<
	@$(AR) $(ARFLAGS) $(LIB) $%; $(RM) $%

$$(LIB)(%.o): $(CONFDIR)/%.c
	$(CC) $(CFLAGS) -c $<
	@$(AR) $(ARFLAGS) $(LIB) $%; $(RM) $%

clean:	$(ARCHCLEAN)
	$(RM) *.a *.o *.i core a.out $(LINTOBJ)

lint: $(CMNDIR)/llib-lsa.c $(PROMLINT)
	@-$(RM) $(LINTOBJ)
	@-(for i in $(CMNSRC) $(CONFSRC) $(CMNDIR)/llib-lsa.c ; do \
		$(CC) -E $(LCOPTS) $$i | \
		$(LINT1) $(LOPTS) >> $(LINTOBJ); done ) 2>&1 | $(LTAIL)

install: install_h $(ARCHINS)

install_h: $$(HDRS)
	install -d -o bin -m 755 $(HDEST)
	install -m 444 $(HDRS) $(HDEST)
