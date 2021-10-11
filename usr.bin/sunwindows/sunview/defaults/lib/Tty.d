SunDefaults_Version 2
;	@(#)Tty.d	1.1	92/07/30 SMI
/Tty 	""

//Ttysw/$Message	"    ~~~~~~ Terminal Emulation subwindow only ~~~~~~"


//Bold_style	"Invert"
	$Enumeration ""
	$Help	"Specifies the method for rendering bold characters."
//Bold_style/None			""
		$Help	"Render emphasized text without any emphasis."
//Bold_style/Offset_X		""
		$Help	"Embolden emphasized text by smearing horizontally."
//Bold_style/Offset_Y		""
	$Help	"Embolden emphasized text by smearing vertically."
//Bold_style/Offset_X_and_Y	""
	$Help	"Embolden emphasized text by smearing vertically and horizontally."
//Bold_style/Offset_XY		""
	$Help	"Embolden emphasized text by smearing diagonally."
//Bold_style/Offset_X_and_XY	""
	$Help	"Embolden emphasized text by smearing horizontally and diagonally."
//Bold_style/Offset_Y_and_XY	""
	$Help	"Embolden emphasized text by smearing vertically and diagonally."
//Bold_style/Offset_X_and_Y_and_XY	""
	$Help	"Embolden emphasized text by smearing horizontally, vertically and diagonally."
//Bold_style/Invert		""
	$Help	"Invert emphasized text"

//Underline_mode	"Enabled"
	$Enumeration ""
	$Help	"Specifies the display style of characters in underline mode."
//Underline_mode/Enabled			""
		$Help	"Underline characters in underline mode."
//Underline_mode/Disabled			""
		$Help	"Do not underline characters in underline mode."
//Underline_mode/Same_as_bold			""
		$Help	"Display characters in underline mode as specified by the Bold_style attribute."

//Inverse_mode	"Enabled"
	$Enumeration ""
	$Help	"Specifies the display style of characters in inverse mode."
//Inverse_mode/Enabled			""
		$Help	"Display characters in inverse mode as inverted (white on black)."
//Inverse_mode/Disabled			""
		$Help	"Display characters in inverse mode as normal characters."
//Inverse_mode/Same_as_bold			""
		$Help	"Display characters in inverse mode as specified by the Bold_style attribute."		

//Retained	"No"
	$Help "Trade-off interactive performance vs. memory consumption in tty subwindows."
	$Enumeration	""
//Retained/Yes	""
	$Help "Improves interactive performance at a slight expense of memory."
//Retained/No	""
	$Help "Lower interactive performance but uses less memory."


//Cmdsw/$Message	"    ~~~~~~ Text-based command subwindow only ~~~~~~"


//Append_only_log		"True"
        $Help           "Specifies whether the entire subwindow is editable, or only just the current line."
        $Enumeration		""
//Append_only_log/True		""
        $Help           "Only the current line is editable."
//Append_only_log/False		""
        $Help           "The entire subwindow is editable."


//Auto_indent		"False"
        $Help           "Specifies whether or not to indent new lines to match indentation of previous line."
        $Enumeration	""
//Auto_indent/True	""
        $Help           "Automatically indent a new line to match the indentation of the previous line."
//Auto_indent/False		""
        $Help           "Do NOT automatically indent a new line to match the indentation of the previous line."
//Auto_indent/Same_as_for_text	""
        $Help           "Create command subwindows with the same Auto_indent setting as for Text."



//Checkpoint_frequency	"0"
	$Help "Specifies number of edits between checkpoints in the cmdtool. 0 implies no checkpointing."

//Control_chars_use_font	"Same_as_for_text"
        $Help           "Specifies how to display control characters."
	$Enumeration				""
//Control_chars_use_font/True			""
	$Help		"Display a control character using corresponding font entry as opposed to ^X."
//Control_chars_use_font/False			""
	$Help		"Display a control character as ^X rather than using corresponding font entry."
//Control_chars_use_font/Same_as_for_text	""
        $Help           "Create command subwindows with the same Control_chars_use_font setting as for Text."


//Insert_makes_caret_visible	"Same_as_for_text"
        $Help           "Specifies how hard to try to keep the caret visible after insertion of a newline."
        $Enumeration	""
//Insert_makes_caret_visible/If_auto_scroll	""
        $Help           "If caret visible before insertion & Auto_scroll_by is positive, make caret visible after."
//Insert_makes_caret_visible/Always		""
        $Help           "The caret is always made visible after any insertion."
//Insert_makes_caret_visible/Same_as_for_text	""
        $Help           "Create command subwindows with the same Insert_makes_caret_visible setting as for Text."

//Text_wraparound_size	"0"
	$Help		"Maximum size of edit log before re-using (wrapping-around) log."
