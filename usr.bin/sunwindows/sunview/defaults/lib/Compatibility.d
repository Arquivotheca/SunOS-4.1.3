SunDefaults_Version 2
;	@(#)Compatibility.d 1.1 92/07/30 SMI
/Compatibility

//Message_01/$Message	"Each of the following items corresponds to some aspect of the SunView user"
//Message_02/$Message	"interface whose *visual* appearance has changed in SunOS 4.0. This defaults"
//Message_03/$Message	"category is provided for backwards visual compatibility. To restore the"
//Message_04/$Message	"visual appearance of SunView to that of SunOS 3.X, simply set the corresponding"
//Message_05/$Message	"defaults variable to `Disabled'."
//Message_06/$Message	"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"


//New_Text_Menu		"Enabled"
	$Help "Enable/disable the new Text menu. For backwards compatibility of the user interface."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Use the new Text menu, with more items in more categories."
	Disabled ""
	Disabled/$Help "Use the old Text menu. For backwards compatibility."

//New_Tty_Menu		"Enabled"
	$Help "Enable/disable the new Tty menu. For backwards compatibility of the user interface."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Use the new Tty menu, with more items."
	Disabled ""
	Disabled/$Help "Use the old Tty menu. For backwards compatibility."


//New_Frame_Menu	"Enabled"
	$Help "Enable/disable the new Frame menu. For backwards compatibility of the user interface."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Use the new Frame menu, with new names for some items."
	Disabled ""
	Disabled/$Help "Use the old Frame menu. For backwards compatibility."


//New_keyboard_accelerators	"Enabled"
	$Help "Enable/disable the new accelerators. For backwards compatibility of the user interface."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Use the new keyboard accelerators. Allows moving the text caret with the keyboard."
	Disabled ""
	Disabled/$Help "Use the old keyboard accelerators (like ctl-D for Delete). For backwards compatibility."


//New_Mailtool_features "Enabled"
	$Help "Enable/disable new Mailtool features. For backwards compatibility of the user interface."
	$Enumeration ""
	Enabled ""
	Enabled/$Help "Use the new simplified panel, multiple composition windows, and fields."
	Disabled ""
	Disabled/$Help "Use the old style panel, and split window for composition."

//Message_9/$Message	"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
//Message_21/$Message	"Finally, there is a new default rootmenu in SunOS 4.0. To restore the SunOS 3.X"
//Message_23/$Message	"rootmenu, switch to the SunView Defaults Category, and set the value"
//Message_24/$Message	"of the defaults variable `Rootmenu_filename' to \"/usr/lib/rootmenu.old\"."









