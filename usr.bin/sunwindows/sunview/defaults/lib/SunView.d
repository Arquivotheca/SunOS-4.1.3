SunDefaults_Version 2
;	@(#)SunView.d	1.1	92/07/30 SMI
/SunView
/SunView/$Help	"Miscellaneous SunView options"

//Click_to_Type	"Disabled"
	$Help "Enable/disable click-to-type model of keyboard interaction."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Click-to-type: left mouse button sets caret; middle mouse button restores caret."
	Disabled ""
	Disabled/$Help "Keyboard follows mouse: cursor over window directs type-in."


//Font	""
	$Help "Name of the default font used to display text. Empty string means default system font."


//Rootmenu_filename ""
	$Help "Name of file containing a customized rootmenu.  Empty string means default rootmenu."


//Icon_gravity	"North"
	$Help "Specifies against which edge of the screen icons are placed."
	$Enumeration ""
	North  ""
	North/$Help "Icons will be placed against top edge of screen."
	South  ""
	South/$Help "Icons will be placed against bottom edge of screen."
	East  ""
	East/$Help "Icons will be placed against right edge of screen."
	West  ""
	West/$Help "Icons will be placed against left edge of screen."


//Icon_close_level "Ahead_of_all"
	$Help "The front-to-back level for the icon when a window closes."
	$Enumeration ""
	Ahead_of_all
	Ahead_of_all/$Help "Icon will be on top of all windows."
 	Ahead_of_icons
	Ahead_of_icons/$Help "Icon will be behind open windows, but ahead of other icons."
	Behind_all
	Behind_all/$Help "Icon will be behind ALL windows, open and iconic."

//Jump_cursor_on_resize	"Disabled"
	$Help "Enable/disable jumping the cursor when the user resizes a window."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "On a resize operation, move the cursor to the window edge."
	Disabled ""
	Disabled/$Help "On a resize operation, move the window edge to the current cursor position."


//Ignore_Optional_Alerts	"Disabled"
	$Help "Enable/disable display of optional (courtesy) alerts."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Display only those alerts which are critical."
	Disabled ""
	Disabled/$Help "Display all alerts, including optional courtesy alerts."


//Alert_Jump_Cursor	"Enabled"
	$Help "Enable/disable jumping the cursor when the an alert is displayed."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "When an alert is displayed, move the cursor to the \"Yes\" button."
	Disabled ""
	Disabled/$Help "When an alert is displayed, do not reposition the cursor."


//Alert_Bell	"1"
	$Help	"Number of times to ring the audible bell when an alert is displayed."


//Audible_Bell	"Enabled"
	$Help "Enable/disable audible bell."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "An audible tone will be emitted upon receipt of a bell command."
	Disabled ""
	Disabled/$Help "No audible tone will be emitted upon receipt of a bell command."


//Visible_Bell	"Enabled"
	$Help "Enable/disable visible bell."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Screen will flash upon receipt of a bell command."
	Disabled ""
	Disabled/$Help "Screen will not flash upon receipt of a bell command."


//Embolden_Labels	"Disabled"
	$Help "Enable/disable displaying all tool labels in boldface."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Tool labels will be bold."
	Disabled ""
	Disabled/$Help "Tool labels will not be bold."


//Ttysubwindow/Retained
	$Help "This item has been moved to /Tty/Retained."


//Root_Pattern	"on"
	$Help "Root window 'pattern'; 'on', 'off', 'gray' or the name of a file produced with iconedit"

//Confirm_Property_Changes	"Disabled"
	$Help "Enable/disable confirmation of changes when a property sheet closes."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Unapplied changes are confirmed when a property sheet closes."
	Disabled ""
	Disabled/$Help "Changes are automatically applied when a property sheet closes."


//Selection_Timeout	"10"
	$Help	"Timeout value in seconds for all selection requests (max=300)."

