#
# @(#)Makefile 1.1 92/07/30 SMI
#

HFILES = icmp_var.h if_ether.h in.h in_pcb.h in_systm.h in_var.h \
	ip.h ip_icmp.h ip_var.h \
	tcp.h tcp_debug.h tcp_fsm.h tcp_seq.h tcp_timer.h \
	tcp_var.h tcpip.h udp.h udp_var.h

HDIR=$(DESTDIR)/usr/include/netinet

install_h: $(HFILES) FRC
	install -d -m 755 $(HDIR)
	install -m 444 $(HFILES) $(HDIR)
FRC:
