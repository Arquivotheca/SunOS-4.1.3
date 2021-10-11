#	@(#)Makefile 1.1 92/07/30 SMI
HDRSPUBLIC = attr.h bitmask.h cms.h cms_colorcube.h cms_grays.h \
	cms_mono.h cms_rainbow.h cms_rgb.h defaults.h filter.h hashfn.h \
	io_stream.h notify.h pixwin.h pw_dblbuf.h pw_util.h rect.h \
	rectlist.h string_utils.h sun.h win_cursor.h win_enum.h \
	win_environ.h win_input.h win_ioctl.h win_keymap.h win_lock.h \
	win_notify.h win_screen.h win_struct.h window_hs.h

HDRSPRIVATE = attr_impl.h cursor_impl.h defaults_impl.h hash.h \
	ndet.h nint.h ntfy.h ndis.h parse.h  win_impl.h sv_malloc.h

include ../../Makefile.master

install_subdir_h:  $(HDRSPUBLIC) $(HDRSPRIVATE)
	for i in ${HDRSPUBLIC}; do ( \
		echo "installing $$i"; \
		${INSTALL} $$i ${DESTDIR}/usr/include/sunwindow; \
	) done
