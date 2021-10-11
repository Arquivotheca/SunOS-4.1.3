SunDefaults_Version 2
;	@(#)Menu.d	1.1	92/07/30 SMI
/Menu			""

//Mess1/$Message	"    ~~~~~~ appearance ~~~~~~"


//Stay_up		"False"
	$Enumeration	""
	$Help		"Enable/disable stay up menus.  First click displays menu, second click causes selection."
	True		""
	True/$Help	"First click displays menu, second click causes selection."
	False		""
	False/$Help	"Menu button down displays menu, button up causes selection."
	False		""
	
//Font			""
	$Help		"Name of the font used to display text in menus. Empty string means default menu font."

//Shadow		"50_percent_gray"
	$Enumeration	""
	$Help		"The type of gray pattern that is used as the shadow for a menu."
	75_percent_gray	""
	50_percent_gray	""
	25_percent_gray	""
	No_Shadow	""

//Margin		"1"
	$Help		"Width of the border surrounding each menu item in pixels."

//Left_margin		"16"
	$Help		"Width of the left margin in pixels."

//Right_margin		"6"
	$Help		"Width of the right margin in pixels."

//Center_string_items	"False"
	$Enumeration	""
	$Help	"Enables/disables centering each item in a menu."
	True		""
	True/$Help	"Center each string item."
	False		""
	False/$Help	"Do NOT center each string item."

//Box_all_items		"False"
	$Enumeration	""
	$Help	"Enables/disables drawing a box around each item in a menu."
	True		""
	True/$Help	"Draw a box around each item."
	False		""
	False/$Help	"Do NOT draw a box around each item."

//Items_in_column_major	"False"
	$Enumeration	""
	$Help	"Enables/disables column major order for 2D menus."
	True		""
	True/$Help	"Items are laid out in columns (similar to ls)."
	False		""
	False/$Help	"Items are laid out in rows (reading order across the screen)."

//Mess2/$Message	""
//Mess3/$Message	"    ~~~~~~ initial and default selection ~~~~~~"

//Initial_Selection	"Default"
	$Enumeration	""
	$Help		"Specifies which item the cursor will be positioned next to when a menu comes up."
	Last_Selection	""
	Last_Selection/$Help "The initial selection will be the previous selection chosen by the user."
	Default		""
	Default/$Help	"The initial selection will be the default selection specified by the application."

//Initial_selection_selected "False"
	$Enumeration	""
	$Help		"Enable/disable positioning the cursor over the initial selection, i.e., selecting it."
	True		""
	True/$Help	"When a menu is displayed, position the cursor over the initial selection."
	False		""
	False/$Help	"When a menu is displayed, position the cursor to the left of the initial selection."

//Mess4/$Message	""
//Mess5/$Message	"    ~~~~~~ restoring cursor ~~~~~~"

//Restore_cursor_after_selection "False"
	$Enumeration	""
	$Help		"Enables/disables jumping the cursor back to its original location if a selection is made."
	True		""
	True/$Help	"Jump the cursor back to its initial location when a selection is made."
	False		""
	False/$Help	"Do NOT jump the cursor when a selection is made."


//Restore_cursor_after_no_selection "False"
	$Enumeration	""
	$Help		"Enables/disables jumping the cursor back to its original location if no selection is made."
	True		""
	True/$Help	"Jump the cursor back to its initial location when no selection is made."
	False		""
	False/$Help	"Do NOT jump the cursor when no selection is made."


//Mess6/$Message	""
//Mess7/$Message	"    ~~~~~~ pullright menus ~~~~~~"

//Pullright_delta       "9999"
	$Help		"Display a pullright menu when the cursor is moved this number of pixels to the right."

//Default_Selection	"Default"
	$Enumeration	""
	$Help		"Specifies the item to be selected when a pull-right menu is brought up."
	Last_Selection	""
	Last_Selection/$Help "When a pull-right menu is brought up, the item selected will be the previous selection."
	Default		""
	Default/$Help	"When a pull-right menu is brought up, the item selected is specified by the application."

//Initial_selection_expanded "True"
	$Enumeration	""
	$Help		"Enable/disable expanding the pullright menu when the initial selection is a pullright item."
	True		""
	True/$Help	"If the initial selection is a pullright item, display the pullright menu."
	False		""
	False/$Help	"Do NOT expand the initial selection."
	False		""

