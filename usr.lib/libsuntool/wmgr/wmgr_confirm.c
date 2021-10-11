#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)wmgr_confirm.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * wmgr_confirm()
 */

#include <suntool/tool_hs.h>
#include <suntool/menu.h>

int
wmgr_confirm(windowfd, text)
        int     windowfd;
        char    *text;
{
        int     returncode = 0;
        struct  prompt prompt;
        struct  inputevent event;
        extern  struct pixfont *pw_pfsysopen();
 
        /*
         * Display a prompt
         */
        rect_construct(&prompt.prt_rect,
            PROMPT_FLEXIBLE, PROMPT_FLEXIBLE,
            PROMPT_FLEXIBLE, PROMPT_FLEXIBLE);
        prompt.prt_font = pw_pfsysopen();
        prompt.prt_text = text;
/******** ALERTS REPLACEMENT FLAG 017 ********/
        (void)menu_prompt(&prompt, &event, windowfd);
        (void)pw_pfsysclose();
        /*
         * See if user wants to continue operation based on last action
         */
        if ((event_action(&event)==SELECT_BUT) && win_inputposevent(&event))     
                returncode = -1;
        return(returncode);
}
