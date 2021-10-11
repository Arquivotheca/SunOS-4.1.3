SunDefaults_Version 2
;	@(#)Text.d	1.1	92/07/30 SMI
/Text			""

//Adjust_is_pending_delete	"False"
	$Enumeration	""
	$Help	"Enables/disables adjusting a selection automatically making it be pending delete."
	True		""
	True/$Help		"Any adjusted selection is pending-delete (not just CTRL-adjusted selections)."
	False		""
	False/$Help		"Only CTRL-selections are pending-delete."

//Again_limit		"1"
	$Help		"Number of user actions which can be repeated."

//Auto_indent		"False"
	$Enumeration	""
        $Help           "Enables/disables indenting new lines to match indentation of previous line."
	True		""
        True/$Help           "Automatically indent a new line to match the indentation of the previous line."
	False		""
        False/$Help           "Do NOT automatically indent a new line to match the indentation of the previous line."

//Auto_scroll_by	"1"
	$Help	"Number of lines to be scrolled when type-in moves the caret below the window."

//Blink_caret		"True"
        $Enumeration		""
        $Help           "Enables/disables a blinking caret."
	True		""
        True/$Help           "The caret blinks."
	False		""
        False/$Help           "The caret is displayed but does not blink."

//Retained	"No"
	$Help "Trade-off interactive performance vs. memory consumption in text subwindows."
	$Enumeration	""
	Yes	""
	Yes/$Help "Improves interactive performance at a slight expense of memory."
	No	""
	No/$Help "Lower interactive performance but uses less memory."


;	Although Checkpoint_frequency is a property of the text facility,
;	this defaults entry only sets checkpointing for the textedit tool.

//Checkpoint_frequency	"0"
	$Help "Specifies number of edits between checkpoints in the textedit tool. 0 implies no checkpointing."

//Confirm_overwrite	"True"
	$Enumeration	""
	$Help		"Enables/disables asking for confirmation before overwriting an existing file."
	True		
	True/$Help	"Ask for confirmation before overwriting."
	False
	False/$Help		"Do NOT ask for confirmation before overwriting."

//Contents		""
	$Help		"Initial contents of the text subwindow. Useful for preloading scratch windows."

//Control_chars_use_font		"False"
	$Enumeration	""
        $Help           "Specifies how to display control characters."
	True			""
	True/$Help		"Display a control character using corresponding font entry as opposed to ^X."
	False			""
	False/$Help		"Display a control character as ^X rather than using corresponding font entry."

//Edit_back_char	"\177"
	$Help		"Key that erases the preceding character -- Del is \\177; BackSpace is \\b."

//Edit_back_word	"\027"
	$Help		"Key that erases the preceding word; \\^W is Control-W"

//Edit_back_line	"\025"
	$Help		"Key that erases the line; \\^U is Control-U"

//Extras_menu_filename ""
	$Help "Name of file containing a customized Extras menu.  Empty string means default Extras menu: /usr/lib/.text_extras_menu"

//Font			""
	$Help		"Name of the font used to display text. Empty string means default SunView font."

//History_limit		"50"
	$Help		"Number of user actions which can be undone."

//Insert_makes_caret_visible	"If_auto_scroll"
        $Help           "Specifies how hard to try to keep the caret visible after any insertion."
        $Enumeration	""
	If_auto_scroll	""
	If_auto_scroll/$Help           "If caret visible before insertion & Auto_scroll_by is positive, make caret visible after."
	Always		""
	Always/$Help           "The caret is always made visible after any insertion."

//Left_margin		"4"
	$Help		"Width of the left margin in pixels."

//Right_margin		"0"
	$Help		"Width of the right margin in pixels."

//Load_file_of_directory		"Is_error"
        $Enumeration			""
	$Help		"Determines whether 'Load file' applied to a directory name is an error."

//Load_file_of_directory/Is_error	""
        $Help           "Using the 'Load file' menu item on a directory name is an error."

//Load_file_of_directory/Is_set_directory	""
        $Help           "The 'Load file' menu item used on a directory name acts like 'Set directory'."

//Long_line_break_mode	"Wrap_char"
	$Enumeration	""
	$Help		"Specifies how to treat lines longer than the width of the window."
	Clip		""
	Clip/$Help	"Clip any material that will not fit in the window."
	Wrap_char	""
	Wrap_char/$Help	"Insert newline before first character that will not fit in the window."
	Wrap_word
	Wrap_word/$Help	"Insert newline before first word that will not fit in the window."

//Lower_context		"0"
	$Help		"Minimum number of lines to maintain between the caret and the bottom of the window. -1 means no auto scrolling."

//Upper_context		"2"
	$Help		"Minimum number of lines to maintain between the start of the selection and the top."

//Memory_Maximum		"20000"
	$Help		"Size of editing memory buffer. 0 means grow the buffer whenever it overflows."	

//Multi_click_space	"3"
	$Help		"Maximum movement (in pixels) between successive multi-click mouse clicks."

//Multi_click_timeout	"390"
	$Help		"Maximum time (in millisecs) between successive multi-click mouse clicks."

;	Scratch_window is actually a property of the textedit tool,
;	not of the text facility in general.

//Scratch_window	"1"
        $Help           "Number of scratch lines in a textedit tool. 0 means no scratch window."

//Scrollable		"True"
	$Enumeration	""
	$Help		"Enables/disables display of vertical scrollbar in text subwindows."
	True		""
	True/$Help	"A text subwindow will have a scrollbar."
	False		""
	False/$Help	"A text subwindow will NOT have a scrollbar."

//Store_changes_file	"True"
	$Help		"Determines whether 'Store' changes the file being edited."
	$Enumeration	""
	True		""
	True/$Help	"'Store' changes the file being edited to be the target of the 'Store'."
	False		""
	False/$Help	"'Store' does not affect the file being edited."

//Store_self_is_save	"True"
	$Help		"Determines whether 'Store' to current file is an error."
	$Enumeration	""
	True		""
	True/$Help	"'Store' to current file is the same as 'Save'."
	False		""
	False/$Help	"'Store' to current file is an error. You must use 'Save' instead of 'Store'."

//Tab_width		"8"
	$Help		"Number of spaces equal to a TAB. (Works best at 8.)"
