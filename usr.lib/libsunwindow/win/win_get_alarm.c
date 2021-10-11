#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)win_get_alarm.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif
#endif

#include	<stdio.h>
#include	<string.h>
#include	<sys/time.h>
#include	<suntool/window.h>

#define	MILLISECONDS	1000
#define	MICROSECONDS	1000000


/*
 *  win_get_alarm.c
 *  This routine gets the WIN_ALARM env var.
 *  and puts the attributes into a Win_alarm structure.
 */
win_get_alarm(wg_alarm)
	Win_alarm	*wg_alarm;
{
	char		*wa_envstr;
	char		*token;
	char		envstr[BUFSIZ];
	char		token_arr[10][BUFSIZ];
	extern	char	*getenv();
	int	i=0;
	int		cmp, err_num, returnval=0;

	wg_alarm->beep_num = 1;
	wg_alarm->flash_num = 1;
	wg_alarm->beep_duration.tv_sec = 1;
	wg_alarm->beep_duration.tv_usec = 0;

	if ( (wa_envstr = getenv ("WINDOW_ALARM")) == NULL ) {
		return(-1);
        }
	
	strcpy(envstr, wa_envstr);
	
	if ((token = strtok (envstr, ":")) != NULL ) {
		i++;
		strcpy(token_arr[i-1],token);
		strcat(token_arr[i-1], "=");
		while ((token = strtok (NULL, ":")) != NULL) {
			i++;
			strcpy(token_arr[i-1],token);
			strcat(token_arr[i-1], "=");
		}
	}
	i = 0;
	while (cmp = strcmp(token_arr[i],"") != 0) {
		err_num = parse_token(token_arr[i],wg_alarm); 
		i++;
		if (returnval == 0) returnval=err_num;
	}
	return(returnval);
}

parse_token(token,pt_alarm)
	char	*token;
	Win_alarm	*pt_alarm;
{
	char	*subtoken;
	int	beeps=0, flashes=0, dur=0;

	if ((subtoken = strtok (token, "=")) != NULL ) {
	    if (strcmp(subtoken,"beeps") == 0) {
               	if ((subtoken = strtok (NULL, "=")) != NULL) {
		    beeps = atoi(subtoken);
		    if (beeps<0) return (-2);
		    pt_alarm->beep_num = beeps;
               	}
	    }
	    else if (strcmp(subtoken,"flashes") == 0) {
		if ((subtoken = strtok (NULL, "=")) != NULL) { 
                    flashes = atoi(subtoken);    
		    if (flashes<0) return (-2);
		    pt_alarm->flash_num = flashes;
                }
	    }
	    else if (strcmp(subtoken,"dur") == 0) 
                if ((subtoken = strtok (NULL, "=")) != NULL) {
                    dur = atoi(subtoken); 
		    if (dur<0) return (-2);
		    if ((dur == 0) && (beeps>0 || flashes>0))  {
			pt_alarm->beep_duration.tv_sec = 1; 
			pt_alarm->beep_duration.tv_usec = 0;
		    }
		    else {
			dur = dur * MILLISECONDS;   
			pt_alarm->beep_duration.tv_sec = dur / MICROSECONDS;
			pt_alarm->beep_duration.tv_usec = dur % MICROSECONDS;
		    }
		}
        }
}
