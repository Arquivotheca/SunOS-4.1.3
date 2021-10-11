SunDefaults_Version 2
;	@(#)Mail.d 1.1 92/07/30 SMI
/Mail 			""
//$Specialformat_to_defaults "mailrc_to_defaults"
//$Defaults_to_specialformat "defaults_to_mailrc"

//Set/Mess1/$Message	"    ~~~~~~ Mailtool related options ~~~~~~"

//Set/allowreversescan	"No"
	$Enumeration	""
	$Help		"Enables/disables processing messages in reverse direction."
	No		"No"
	No/$Help	"Messages will be processed in the forward direction, i.e., first in, first processed."
	Yes		"Yes"
	Yes/$Help	"Messages will be processed in the reverse direction, i.e., last in, first processed."

//Set/alwaysusepopup	"No"
	$Enumeration	""
	$Help		"Controls creation of message composition window."
	No		"No"
	No/$Help	"Create message composition window by splitting message viewing window."
	Yes		"Yes"
	Yes/$Help	"Create message composition window by popping up a separate frame."
	
//Set/askbcc		"No"
	$Enumeration	""
	$Help		"Enables/disables prompting user for 'bcc' field when sending."
	No		"No"
	No/$Help	"Do NOT automatically prompt user for 'bcc' field when sending."
	Yes		"Yes"
	Yes/$Help	"Automatically prompt user for 'bcc' field when sending."

//Set/bell		"0"
	$Help		"Number of times to ring the audible bell when new mail arrives."

//Set/disablefields	"No"
	$Enumeration	""
	$Help		"Enables/disables use of text fields in message composition window."
	No		"No"
	No/$Help	"Use fields. Allows user to advance to next (previous) field via CTRL-TAB (SHIFT-CTRL-TAB)."
	Yes		"Yes"
	Yes/$Help	"Do not use fields."

//Set/editmessagewindow	"Yes"
	$Enumeration	""
	$Help		"Enables/disables editing of messages in message subwindow."
	No	"No"
	No/$Help	"The first edit in the message subwindow will ask for confirmation."
	Yes		"Yes"
	Yes/$Help	"Allows editing of message subwindow. Confirmation requested when edits are saved."
	$Always_Save	"Yes"
	
//Set/expert		"No"
	$Enumeration	""
	$Help		"Enables/disables expert mode, i.e., no confirmations."
	No		"No"
	No/$Help	"Disable expert mode. Request confirmations."
	Yes		"Yes"
	Yes/$Help	"Operate in expert mode. Do not request confirmations."
	
//Set/filemenu
	$Help		"List of files to initialize the 'File:' menu, e.g,. \"+mbox +trash\"."

//Set/filemenusize	"10"
	$Help		"Maximum number of files cached in the File: menu."

//Set/flash		"0"
	$Help		"Number of times to flash the Mailtool window or icon when new mail arrives."

//Set/interval		"300"
	$Help		"Number of seconds to wait before checking again for new mail."

//Set/moveinputfocus	"No"
	$Enumeration	""
	$Help		"Specifies whether or not to move input focus to composition window when it is created."
	No		"No"
	No/$Help	"Only move input focus if it is already in one of the mailtool's subwindows."
	Yes		"Yes"
	Yes/$Help	"Always move input focus. Enables user to immediately start typing response."

//Set/msgpercent	"50"
	$Help		"Percent of message window to use for displaying current message while composing a new one."

//Set/printmail
	$Help		"Commandline to be used for printing, e.g., 'pr -h mail | lpr -Pimagen'." Default is "lpr -p".

//Set/trash
	$Help		"Name of trash folder. Deleted messages stay here until you hit 'Done'."


//Set/Mess2/$Message	""
//Set/Mess3/$Message	"    ~~~~~~ options relating to appearance of mailtool ~~~~~~"
//Set/headerlines	"10"
	$Help		"Number of lines in header subwindow."

//Set/maillines		"30"
	$Help		"Number of lines in message subwindow."

//Set/popuplines		"30"
	$Help		"Number of lines in popup message composition subwindow."

//Set/Mess6/$Message	""
//Set/Mess7/$Message	"    ~~~~~~ options affecting both Mailtool and Mail ~~~~~~"
	
//Set/allnet		"No"
	$Enumeration	""
	$Help		"Affects how comparisons of recipient addresses are performed."
	No		"No"
	No/$Help	"Comparisons of recipient addresses use the entire recipient string."
	Yes		"Yes"
	Yes/$Help	"Comparisons of recipient addresses use only the base user name."

//Set/alwaysignore	"No"
	$Enumeration	""
	$Help		"Determines which commands are affected by 'ignore' settings."
	No		"No"
	No/$Help	"Only the 'print' command is affected by 'ignore' settings."
	Yes		"Yes"
	Yes/$Help	"All commands are affected by the 'ignore' settings, e.g., save, copy, etc."

//Set/append		"Yes"
	$Enumeration	""
	$Help		"Specifies whether messages are appended or prepended to 'mbox'."
	No		"No"
	No/$Help	"Prepend messages at beginning of 'mbox'."
	Yes		"Yes"
	Yes/$Help	"Append messages to end of 'mbox'."
	Yes		"Yes"

//Set/ask		"No"
	$Enumeration	""
	$Help		"No longer implemented. Use 'asksub'."
	Yes		"Yes"
	No		"No"

//Set/askcc		"No"
	$Enumeration	""
	$Help		"Enables/disables prompting user for 'cc' field when sending."
	No		"No"
	No/$Help	"Do NOT automatically prompt user for 'cc' field when sending."
	Yes		"Yes"
	Yes/$Help	"Automatically prompt user for 'cc' field when sending."

//Set/asksub		"Yes"
	$Enumeration	""
	$Help		"Enables/disables prompting user for 'subject' field when sending."
	No		"No"
	No/$Help	"Do NOT automatically prompt user for 'subject' field when sending."
	Yes		"Yes"
	Yes/$Help	"Automatically prompt user for 'subject' field when sending."

//Set/autoprint		"No"
	$Enumeration	""
	$Help		"Enables/disables displaying the next message when current message is deleted."
	No		"No"
	No/$Help	"Do NOT automatically display next message when current message is deleted."
	Yes		"Yes"
	Yes/$Help	"Automatically display next message when current message is deleted."

//Set/DEAD		"~/dead.letter"
	$Help		"Full path name of file where partial messages are stored in case of interrupt or delivery error."

//Set/folder
	$Help		"Directory in which to store mail folders."

//Set/hold		"Yes"
	$Enumeration	""
	$Help		"Enables/disables preserving messages that are read in the mailbox."
	No		"No"
	No/$Help	"Move messages that are read into 'mbox'."
	Yes		"Yes"
	Yes/$Help	"Hold messages in mailbox."
	$Always_Save	"Yes"

//Set/indentprefix	"	"
	$Help		"Inserted in front of message when message is indented. Defaults to tab."

//Set/keep		"No"
	$Enumeration	""
	$Help		"Enables/disables deleting system mailbox when empty."
	No		"No"
	No/$Help	"When mailbox is empty, delete it."
	Yes		"Yes""
	Yes/$Help	"When mailbox is empty, truncate it to zero length."

//Set/keepsave		"No"
	$Enumeration	""
	$Help		"Enables/disables deleting messages that are saved in folders."
	No		"No"
	No/$Help	"Delete messages that are saved in folders."
	Yes		"Yes"
	Yes/$Help	"Do NOT delete messages that are saved in folders."

//Set/LISTER		"ls -F"
	$Help		"Command (and options) for listing folders. 'ls -F' shows subdirectories as pullrights."

//Set/MBOX		"~/mbox"
	$Help		"Full path name of file in which to save messages that have been read. See 'hold' option."

//Set/metoo		"No"
	$Enumeration	""
	$Help		"Enables/disables including the sender if listed in an alias."
	No		"No"
	No/$Help	"Do NOT include sender when listed in an alias."
	Yes		"Yes"
	Yes/$Help	"Include sender when listed in an alias."

//Set/onehop		"No"
	$Help		"Enables/disables alteration of addresses when Replying, enabling direct response where possible."
	$Enumeration	""
	No		"No"
	No/$Help	"Recipients' addresses are altered to be relative to originating author's machine."
	Yes		"Yes"
	Yes/$Help	"Recipients' addresses are not altered."

//Set/outfolder		"No"
	$Enumeration	""
	$Help		"Files to record outgoing mail are located in the 'folder' directory. See 'record' option."
	No		"No"
	No/$Help	"Do NOT save outgoing mail.  See 'Set/record' below."
	Yes		"Yes"
	Yes/$Help	"Save outgoing mail.  See 'Set/record' below."
//Set/record
	$Help		"File in which to save all outgoing mail."
//Set/replyall		"No"
	$Enumeration	""
	$Help		"Reverse action of reply and Reply."
	No		"No"
	No/$Help	"'r' means reply to just to sender, 'R' means reply to all recipients."
	Yes		"Yes"
	Yes/$Help	"'r' means reply to all recipients, 'R' means reply to just to sender."

//Set/save		"Yes"
	$Enumeration	""
	$Help		"Enables/disables saving partial messages in the file specified by the value of DEAD."
	No		"No"
	No/$Help	"Do NOT save partial messages in file specified by the value of DEAD."
	Yes		"Yes"
	Yes/$Help	"Save partial messages in file specified by the value of DEAD."

//Set/sendmail		"sendmail"
	$Help		"Alternate command for delivering messages."

//Set/showto		"No"
	$Enumeration	""
	$Help		"Enables/disables showing name of recipient instead of author when message is from you."
	No		"No"
	No/$Help	"Always show sender in the header line"
	Yes		"Yes"
	Yes/$Help	"If message was sent by the reader, show recipient instead of sender in the header line"

//Set/Mess8/$Message	""
//Set/Mess9/$Message	"    ~~~~~~ options that affect only the program 'Mail'  ~~~~~~"
	
//Set/bang		"No"
	$Enumeration	""
	$Help		"Enable/disable special-casing of exclamation points (!) in shell escape commands."
	No		"No"
	No/$Help	"Enable special-casing of exclamation points (!) in shell escape commands."
	Yes		"Yes"
	Yes/$Help	"Disable special-casing of exclamation points (!) in shell escape commands."

//Set/cmd
	$Help		"Default command for the pipe command. (Wizards option.)"

//Set/conv
	$Help		"Address style for conversion of uucp addresses."

//Set/crt
	$Help		"Pipe messages having more than this number of lines through command specified by 'Pager'."

//Set/dot		"Yes"
	$Enumeration	""
	$Help		"Accept '.' alone on line to terminate message."
	No		"No"
	Yes		"Yes"

//Set/EDITOR		"ex"
	$Help		"The command to run when the edit or '~e' command is used."

//Set/escape
	$Help		"Escape character to be used instead of ~."

//Set/header		"Yes"
	$Enumeration	""
	$Help		"Enable/disable printing of the header summary when entering mail."
	No		"No"
	No/$Help	"Do not print header summary when entering mail."
	Yes		"Yes"
	Yes/$Help	"Print header summary when entering mail."

//Set/ignore		"No"
	$Enumeration	""
	$Help		"Enable/disable ignoring interrupts while entering messages. Handy for noisy dialup lines."
	No		"No"
	No/$Help	"Notice ^C's (RUBOUT) while sending mail."
	Yes		"Yes"
	Yes/$Help	"Ignore ^C's (RUBOUT) while sending mail."

//Set/ignoreeof		"No"
	$Enumeration	""
	$Help		"Enable/disable terminating messages/command inputs with ^D."
	No		"No"
	No/$Help	"Terminate letters/command input with ^D. "
	Yes		"Yes"
	Yes/$Help	"Input must be terminated by a period on a line by itself, or by the '~.' command."


//Set/page		"No"		
	$Enumeration	""
	$Help		"Enables/disables inserting form feed after each message sent through the pipe."
	No		"No"
	No/$Help	"Do NOT insert form feed after each message."
	Yes		"Yes"
	Yes/$Help	"Insert form feed after each message."

//Set/PAGER		"more"
	$Help		"Command to be used as a filter for paginating output. See 'crt' option."

//Set/prompt		"& "
	$Help		"Command mode prompt."

//Set/quiet		"No"
	$Enumeration	""
	$Help		"Enables/disables suppressing printing of opening message and version number."
	No		"No"
	No/$Help	"Print opening message and version number when entering Mail."
	Yes		"Yes"
	Yes/$Help	"Suppress printing of opening message and Mail version."

//Set/screen
	$Help		"Sets the number of lines in a screen-full of headers for the headers command."

//Set/sendwait		"No"
	$Enumeration	""
	$Help		"Enables/disables waiting for background mailer to finish before returning."
	No		"No"
	No/$Help	"Do not wait for background mailer to finish before returning."
	Yes		"Yes"
	Yes/$Help	"Wait for background mailer to finish before returning."

//Set/SHELL		"sh"
	$Help		"Name of the command interpreter to be used in the '!' command and '~!' escape."

//Set/sign
	$Help		"Variable inserted into the text of the message when '~a' (autograph) command is given."

//Set/toplines		"5"
	$Help		"Number of lines of header to be printed by the 'top' command."
	
//Set/verbose		"No"
	$Enumeration	""
	$Help		"Enables/disables invoking 'sendmail' with the '-v'."
	No		"No"
	No/$Help	"Do NOT 'sendmail' with the '-v' flag."
	Yes		"Yes"
	Yes/$Help	"Invoke 'sendmail' with the '-v' flag."

//Set/VISUAL		"vi"
	$Help		"Name of screen editor to be used with visual or '~v' command."




