/*	"@(#)procs.h 1.1 92/07/30 Copyright Sun Micro"	*/

/************************ external functions **************************/
/*
 * in panel.c
 */
extern init_control();

/*
 * in panel_procs.c
 */
extern start_proc();
extern stop_proc();
extern log_files_menu_proc();
extern option_files_proc();
extern options_proc();
extern quit_proc();
extern reset_proc();
extern pscrn_proc();
extern ttime_proc();
extern eeprom_proc();
extern cpu_proc();
extern suspend_proc();
extern resume_proc();

extern interven_proc();
extern manuf_proc();
extern select_proc();
