#
#	@(#)Makefile 1.1 92/07/30 SMI;
#

.DEFAULT:
	sccs get -G$@ $@

STD	= sundiag_rpcnums.h lfill_lcheck.h dev_io.h \
	  dev_io_msgs.h tape_strings.h disk_strings.h mtio.h fb_strings.h \
	  prestoioctl.h
ATS	= sundiag_ats.h tcm_prognum.h ats_sundiag.h atmsg_fifo.h
XDR	= dtm_tcm.h tcm_dtm.h amp.h
TESTLIB = sdrtns.h testnames.h

all: $(STD) $(ATS) $(XDR) $(TESTLIB)

install: all

clean:

lint:

info: FRC
	sccs info

FRC:
