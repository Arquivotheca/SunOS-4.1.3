#
# @(#)Makefile 1.1 92/07/30 SMI
#

.KEEP_STATE:

DESTDIR =
INSDIR = $(DESTDIR)/usr/demo/SOUND
MANDIR = $(INSDIR)/man3

LIBRARY = libaudio.a
LINTLIB = llib-llibaudio.ln
.PRECIOUS: $(LIBRARY)

# source files in this directory
LIBSRCS = filehdr.c device_ctl.c hdr_misc.c ulaw2linear.c
LIBHDRS = libaudio_impl.h

LIBMANS = audio_intro.3 audio_hdr.3 audio_filehdr.3 audio_misc.3 \
		audio_device.3 audio_convert.3 audio_ulaw2linear.3


# 'make install' targets
INSFILES = $(MANDIR) $(LIBMANS:%=$(MANDIR)/%) $(LINTLIB:%=$(INSDIR)/%)


# default rule builds the library
all default: $(LIBHDRS) $(LIBRARY)

# install man pages and lint library only
install: $(INSFILES) $(LINTLIB)


# rules to build the library
INCLUDE =
CPPFLAGS = -I. -I../multimedia $(INCLUDE)
CFLAGS = -O
LIBOBJS = $(LIBSRCS:%.c=%.o)

$(LIBRARY): $$@($(LIBOBJS))
	$(AR) rc $@ $?
	ranlib $@
	$(RM) $?

# this rule is necessary because the default library rules are broken
.c.a:
	$(COMPILE.c) -o $% $<


# rules to build the lint library
lint: $(LINTLIB)

LIBLINTS = $(LIBSRCS:%.c=%.ln)

$(LINTLIB): $(LIBHDRS) $(LIBSRCS)
	@$(MAKE) $(MFLAGS) $(LIBLINTS) INCLUDE=$(INCLUDE)
	@cat $(LIBLINTS) > $@
	@echo Lint pass 2:
	@$(LINT.c) -u $@ 
	@$(RM) $(LIBLINTS)


# ulaw2linear.c is a file derived algorithmically
ulaw2linear.c: gen_ulaw2linear
	gen_ulaw2linear > $@


# rules for testing the conversion macros and tables
TESTSRC = test_ulaw2linear.c test_dbl2int.c
TESTS = $(TESTSRC:%.c=%)
$(TESTS) :=	LDLIBS = $(LIBRARY) -lm

test: $(LIBRARY) $(TESTS)
	@for i in $(TESTS); do \
		$$i; \
	done


# rules for installing files from this directory
OWNER = bin
INSTALL = install -o $(OWNER)

$(INSDIR)/%: %
	$(INSTALL) -m 644 $< $(INSDIR)

$(MANDIR)/%: %
	$(INSTALL) -m 644 $< $(MANDIR)

# rule to ensure target directories exist
$(INSDIR) $(MANDIR):
	?$(INSTALL) -d -m 755 $@


clean:
	$(RM) $(LIBRARY) $(TESTS) gen_ulaw2linear ulaw2linear.c *.o *.ln
