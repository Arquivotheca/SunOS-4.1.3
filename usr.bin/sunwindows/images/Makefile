# @(#)Makefile 1.1 92/07/30 SMI

SRC =	abc.pr black.cursor blank.pr bob.icon boggle.icon box.pr \
	bullseye.cursor canfield.icon canvas_demo.icon cardback.icon \
	chess.clock.icon chesstool.icon circle.pr circle_0.pr \
	circle_25.pr circle_50.pr circle_75.pr circle_black.pr \
	circle_root.pr clock.icon clock.rom.icon cmdtool.icon \
	cmrb14.pr cmrr14.pr confirm.cursor confirm.pr \
	confirm_left.pr confirm_middle.pr confirm_right.pr core_eye.icon \
	cursor_demo.icon cycle.pr dbxtool.icon dead.icon \
	defaultsedit.icon down_arrow.pr fontedit.icon gallant19.pr \
	gallantr19.pr gammon.icon gfxtool.icon gr_25.cursor \
	gr_50.cursor gr_75.cursor gr_root.cursor grid.pr \
	hello_world.icon hglass.cursor iconedit.icon indentdefaults.icon \
	jump1.icon jump2.icon jump3.icon jump4.icon \
	jump5.icon jump6.icon k1.pr k2.pr \
	k3.pr k4.pr k5.pr line.pr \
	life.icon \
	lockscreen.icon mail.icon mailseen.icon mouse_left_hand.pr \
	mouse_left_hand_grey.pr mouse_mid_hand.pr mouse_mid_hand_grey.pr move.cursor \
	move_h.cursor move_v.cursor nautilus.icon nomail.icon \
	none.cursor off.pr on.pr overview.icon \
	painting_hand.pr panel_button.pr panel_check_choice.pr panel_check_off.pr \
	panel_check_on.pr panel_choose_many.pr panel_choose_one.pr panel_flush.pr \
	panel_choice_on.pr panel_choice_off.pr panel_right_arrow.pr \
	panel_left_arrow.pr panel_menu_on.pr panel_switch.pr panel_text.pr \
	point.pr rspeedometer.icon screenb12.pr screenb14.pr \
	screenr11.pr screenr12.pr screenr14.pr screenr7.pr \
	scrollbar_demo.icon sick.icon size_pointer.pr \
	speedometer.icon square_0.pr \
	square_17.pr square_25.pr square_50.pr square_75.pr \
	square_80.pr square_black.pr square_root.pr stretchNE.cursor \
	stretchNW.cursor stretchSE.cursor stretchSW.cursor stretch_h.cursor \
	stretch_v.cursor sun30.icon sunview_manual.icon switcher.icon \
	tektool.icon template.icon terminal.icon textedit.icon \
	traffic.icon tri_down.pr tri_left.pr tri_right.pr \
	tri_up.pr white.pr compose.icon compose.image drawer.image \
	folder.image letter.image mailtool_busy.icon menu.image \
	newmail.image next.image nomail.image prev.image printer.image \
	reply.icon reply.image save.image show.image stop.icon \
	trash.image trashfull.image \
	compose.3Dimage		load.3Dimage		prev.3Dimage \
	delete.3Dimage		misc.3Dimage		print.3Dimage \
	delete_full.3Dimage	new_mail_open.3Dimage	reply.3Dimage \
	done.3Dimage		next.3Dimage		show.3Dimage \
	help.3Dimage		no_mail.3Dimage		store.3Dimage \
	alert_bang32.pr alert_qmark32.pr help.icon empty_letter.icon \
	leftswoosh.icon rightswoosh.icon

all: $(SRC)

IMAGEDIR=$(DESTDIR)/usr/include/images

clean:
	$(RM) $(SRC)

# include the common stuff from ../Makefile.master
include ../Makefile.master

fast_install: install
install: $(SRC)
	install -d -o bin -m 755 $(IMAGEDIR)
	install -c -o bin -m 644 $(SRC) $(IMAGEDIR)
	$(RM) $(IMAGEDIR)/clocktool.icon
	ln -s clock.icon $(IMAGEDIR)/clocktool.icon
	$(RM) $(IMAGEDIR)/clocktool.rom.icon
	ln -s clock.rom.icon $(IMAGEDIR)/clocktool.rom.icon
