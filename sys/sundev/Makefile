#
# @(#)Makefile 1.1 92/07/30 SMI
#
HFILES= arreg.h \
	bw2reg.h \
	cg4reg.h cg8reg.h cg9reg.h \
	dbio.h dmctl.h \
	fdreg.h fpareg.h \
	idvar.h ipdev.h ipi3.h ipi_chan.h ipi_driver.h \
	ipi_error.h ipi_trace.h ipvar.h isdev.h isvar.h \
	kbd.h kbdreg.h kbio.h \
	lightpenreg.h \
	mbvar.h mcpcmd.h mcpcom.h mcpreg.h msio.h msreg.h mtireg.h mtivar.h \
	openpromio.h \
	p4reg.h ppreg.h \
	ramdac.h cg6reg.h \
	screg.h scsi.h sdreg.h sereg.h sfreg.h sireg.h skyreg.h \
	smreg.h srreg.h streg.h swreg.h \
	tmreg.h todreg.h tv1reg.h \
	vpcreg.h vpreg.h vuid_event.h vuid_queue.h vuid_state.h vuid_store.h \
	xdcreg.h xderr.h xdreg.h xdvar.h xtreg.h xycom.h xycreg.h \
	xyerr.h xyreg.h xyvar.h \
	zscom.h zsreg.h zsvar.h

HDIR=$(DESTDIR)/usr/include/sundev

install_h: $(HFILES)
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
