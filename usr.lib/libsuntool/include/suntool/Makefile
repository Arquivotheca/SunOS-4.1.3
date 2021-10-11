#	@(#)Makefile 1.1 92/07/30 SMI
HDRSPUBLIC = alert.h canvas.h emptysw.h expand_name.h frame.h fullscreen.h \
	gfx_hs.h gfxsw.h help.h icon.h icon_load.h image.h menu.h msgsw.h \
	panel.h scrollbar.h selection.h selection_attributes.h \
	selection_svc.h seln.h sunview.h text.h textsw.h tool.h tool_hs.h \
	tool_struct.h tty.h ttysw.h ttytlsw.h walkmenu.h window.h wmgr.h 

HDRSPRIVATE =  alert_impl.h canvas_impl.h cheap_text.h charimage.h \
	charscreen.h entity_interpreter.h entity_stream.h entity_view.h \
	ev_impl.h finger_table.h frame_impl.h gfxswimpl.h     \
	image_impl.h  panel_impl.h primal.h ps_impl.h scrollbar_impl.h \
	selection_impl.h text_obj.h textsw_impl.h tool_commons.h \
	tool_impl.h ttyansi.h ttysw_impl.h ttytlsw_impl.h walkmenu_impl.h \
	window_impl.h walkmenu_impl.h wmgr_policy.h    

include ../../Makefile.master

install_subdir_h:  $(HDRSPUBLIC) $(HDRSPRIVATE)
	for i in ${HDRSPUBLIC}; do ( \
		echo "installing $$i"; \
		${INSTALL}  $$i ${DESTDIR}/usr/include/suntool; \
	) done
