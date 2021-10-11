 
# @(#)Makefile	1.1 92/07/30 SMI


BINS=	video
OBJS=	main.o file_panel.o fileio.o video_cursor.o video_panel.o \
	video_adjust.o slider.o panel_util.o video_setup.o \
	initialize_items.o video_poll.o de_interlace.o video_cbars.o \
	save_postscript.o

SOURCES=	main.c file_panel.c fileio.c video_cursor.c video_panel.c \
		video_adjust.c slider.c panel_util.c video_setup.c \
		initialize_items.c video_poll.c de_interlace.c video_cbars.c \
		save_postscript.c
SUBDIR=		LASERDISC

HDRS=		video.h video_icon.h init_item.h video_impl.h video_mat.h
INST_HDRS=	video.h video_icon.h init_item.h video_mat.h
LOCAL_HDRS=	video_impl.h
LIBVIDEOBJS=	video_win.o video_pr.o
LIBPIXRECT=	-lpixrect
LIBSUNWINDOW=	-lsunwindow
LIBSUNTOOL=	-lsuntool
LIBVIDEO=	libvideo.a
LIBS=		$(LIBVIDEO) $(LIBSUNTOOL) $(LIBSUNWINDOW) $(LIBPIXRECT) -lc -lm
INC=		-I. -I../../sys
CFLAGS=		-O $(INC) -DFLAMINGO -DSTANDALONE
DEST_SRC=       $(DESTDIR)/usr/demo/SUNVIDEO
DEST_BIN=	$(DESTDIR)/usr/demo/SUNVIDEO
DEST_LIB=	$(DESTDIR)/usr/demo/SUNVIDEO

.KEEP_STATE:

all:	$(HDRS) libvideo.a $(BINS)
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS}); \
		done

	

video:	$(HDRS) $(OBJS) $(LIBVIDEO)
	$(LINK.c) -o $@ $(OBJS) $(LIBS)

libvideo.a:	$(LOCAL_HDRS) $(LIBVIDEOBJS)
	ar lrc $@ $?
	ranlib $@

clean:
	rm -f *.o core video vidcal libvideo.a; \
	set -x;for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done


install:	$(SOURCES) $(INST_HDRS) $(LIBVIDEO) $(BINS) Makefile.install
	install -d -m 755 $(DEST_SRC); \
	install -m 444 $(SOURCES) $(DEST_SRC); \
	install -m 444 $(INST_HDRS) $(DEST_SRC); \
	install -m 644 $(LIBVIDEO) $(DEST_LIB); \
	install -m 555 $(BINS) $(DEST_BIN); \
	install -m 444 Makefile.install $(DEST_SRC)/Makefile; \
	ranlib	$(DEST_LIB)/$(LIBVIDEO); \
	chmod	444 $(DEST_LIB)/$(LIBVIDEO); \
	set -x; for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); \
		done

