SunDefaults_Version 2
;	@(#) Input.d 1.1	92/07/30 SMI
/Input	""

//$Specialformat_to_defaults "defaults_from_input"
//$Defaults_to_specialformat "input_from_defaults"

//Keymap_Directory	"/usr/lib/keymaps"
	$Help	"Directory in which kernel and sunview keymap configuration files reside"

//SunView_Keys	"Yes"
	$Enumeration ""
	$Help	"Are the standard SunView functions emitted from function keys?"
	Yes	""
	Yes/$Help	"Some function keys emit editing & window-manipulation codes."
	No	""
	No/$Help	"SunView function-keys emit T11-T15 and L11-L15 instead."

//Arrow_Keys	"Yes"
	$Enumeration ""
	$Help	"Do arrow-keys emit cursor-motion codes?"
	Yes	""
	Yes/$Help	"Arrow keys emit cursor-motion codes."
	No	""
	No/$Help	"Arrow keys emit function-key codes."

//Left_Handed	"No"
	$Enumeration ""
	$Help	"Are SunView functions on the right pad, and mouse buttons reversed?"
	Yes	""
	Yes/$Help	"SunView functions on the right pad; mouse buttons emit R, M, L."
	No	""
	No/$Help	"SunView functions on the left pad; mouse buttons emit L, M, R."

//Jitter_Filter	"Off"
	$Enumeration ""
	$Help	"Are small mouse motions suppressed to prevent cursor jitter?"
	On	""
	On/$Help	"Small mouse motions are suppressed to prevent cursor jitter (appropriate for some 100Us)."
	Off	""
	Off/$Help	"No filtering of small mouse motions (appropriate for Sun-2s and later machines)."

//Speed_Enforced	"No"
	$Enumeration ""
	$Help	"Is the mouse speed limit below enforced?"
	Yes	""
	Yes/$Help	"Large mouse motions are discarded (appropriate for some 100Us)."
	No	""
	No/$Help	"Large mouse motions are not discarded (appropriate for Sun-2s and later machines)."

//Speed_Limit	"48"
	$Help	"Motions greater than this value in a single sample are ignored if Speed_Enforced (above) is true."

//Mess1/$Message	"Mouse motion scaling:"
//Mess2/$Message	"    Motions up to a given ceiling are multiplied by the corresponding"
//Mess3/$Message	"    factor.  Pairs should be in ascending order on ceiling."

//1st_ceiling	"65535"
	$Help	"The smallest set of motions to be scaled."
//1st_factor	"2"
	$Help	"Multiply motions up to 1st_ceiling by this amount."

//2nd_ceiling	"0"
	$Help	"The second set of motions to be scaled."
//2nd_factor	"0"
	$Help	"Multiply motions up to 2nd_ceiling by this amount."

//3rd_ceiling	"0"
	$Help	"The third set of motions to be scaled."
//3rd_factor	"0"
	$Help	"Multiply motions up to 3rd_ceiling by this amount."

//4th_ceiling	"0"
	$Help	"The fourth set of motions to be scaled."
//4th_factor	"0"
	$Help	"Multiply motions up to 4th_ceiling by this amount."

//Mess4/$Message	"Press this button to set defaults parameters to current kernel values."

//$Setup_program "defaults_from_input"
	$Help	"A program which sets defaults input parameters from current kernel values."
