/*	@(#)sony_laser.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * Include all of the laser header files and define single byte macros
 */
#ifndef laser_DEFINED
#define laser_DEFINED

/*
 * Include other header files
 */
#include <stdio.h>
#include <sys/types.h>
#include "sony_codes.h"

/*
 * Single byte macros
 */
extern u_char sony_audio_mute_on();
extern u_char sony_audio_mute_off();
extern u_char sony_clear_entry();
extern u_char sony_ch1_on();		
extern u_char sony_ch1_off();		
extern u_char sony_ch2_on();		
extern u_char sony_ch2_off();		
extern u_char sony_chapter_mode();	
extern u_char sony_clear_all();	
extern u_char sony_cont();			
extern u_char sony_cx_on();		
extern u_char sony_cx_off();		
extern u_char sony_eject_enable();	
extern u_char sony_eject_disable();
extern u_char sony_enter();		
extern u_char sony_f_fast();		
extern u_char sony_r_fast();		
extern u_char sony_f_play();		
extern u_char sony_r_play();		
extern u_char sony_f_scan();		
extern u_char sony_r_scan();		
extern u_char sony_f_slow();		
extern u_char sony_r_slow();		
extern u_char sony_f_step_still();	
extern u_char sony_r_step_still();	
extern u_char sony_frame_mode();	
extern u_char sony_index_on();		
extern u_char sony_index_off();	
extern u_char sony_memory();		
extern u_char sony_menu();			
extern u_char sony_non_cf_play();	
extern u_char sony_psc_enable();	
extern u_char sony_psc_disable();	
extern u_char sony_still();		
extern u_char sony_stop();			
extern u_char sony_video_on();		
extern u_char sony_video_off();	
extern u_char sony_read();
extern int sony_clv_disk();

/*
 * mbyte declarations
 */
extern u_char sony_search(), sony_repeat();
extern u_char sony_motor_on(), sony_motor_off();
extern u_char sony_eject();
extern u_char sony_f_step(), sony_r_step(), sony_f_track_jump();
extern u_char sony_r_track_jump();
extern void sony_status_inq(), sony_users_code_inq(), sony_rom_version();
extern char sony_m_search(), sony_mark_set();
extern char sony_replay_seq(), sony_replay_vspeed();
extern void sony_chapter_inq(), sony_addr_inq();

/*
 * Serial line open, close
 */
extern FILE *sony_open();
extern void sony_close();

/*
 * Utilites
 */
extern u_char *sony_itoa();
extern void sony_settimeout(), sony_blockingread();

#endif laser_DEFINED
