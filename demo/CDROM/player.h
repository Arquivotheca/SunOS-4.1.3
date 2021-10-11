/* @(#)player.h 1.1 92/07/30 Copyr 1989, 1990 Sun Microsystem, Inc. */

#define	DPRINTF(L)	if(debug >= L) printf

#define MAX_TRACKS	99

extern	char	*ProgName;
extern	char	*Version;
extern	int	debug;
extern	int	fd;
extern	int	ms_fd;
extern	int	kbd_fd;
extern	int	current_track;
extern	int	num_tracks;
extern 	int	playing;
extern	int	paused;
extern	int	ejected;
extern	int	idled;
extern	int	rewinding;
extern	int	forwarding;
extern	int	play_mode;
extern	int	idle_mode;
extern	int	idle_time;
extern	int	prop_showing;
extern	int	left_balance;
extern	int	right_balance;
extern	int	volume;
extern	int	rew_delta;
extern	int	ff_delta;
extern	int	time_remain_flag;
extern	int	last_guage_value;
extern	Msf	total_msf;
extern	Toc	toc;
extern	int	mem_ntracks;
extern	int	mem_play[MAX_TRACKS];
extern	int	mem_current_track;
extern	int	mem_buttons_dirty;
extern	Panel	panel;
extern	Panel_item	track_items[MAX_TRACKS];
extern	Panel_item	msg_item;
extern	struct	tm	*tmp;
extern	struct	msf	mem_total_msf;
extern	struct	msf	mem_cur_msf;

void	pause_proc();
void	add_memory_track();
void	delete_memory_track();
void	display_mem_tracks();
void	set_remain_guage();
void	mem_cleanup();
void	check_idle();
void	shuffle();

#define	TIME_REMAIN_TRACK	0
#define TIME_REMAIN_DISC	1

#define PLAY_MODE_NORMAL	0
#define PLAY_MODE_MEMORY	1
#define	PLAY_MODE_RANDOM	2
