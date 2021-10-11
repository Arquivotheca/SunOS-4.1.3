#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)stty_from_defaults.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *
 *	stty_from_defaults: set tty options from defaults database
 */

#include <sys/file.h>
#include <sgtty.h>
#include <sys/ioctl.h>
#include <sunwindow/defaults.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

		
#ifdef STANDALONE
main()
#else
stty_from_defaults_main()
#endif
{
        int              fd;
        struct sgttyb    sgb;
	struct ltchars	 ltc;
	char		*def_str;

	fd = open("/dev/tty", O_RDONLY, 0);
        if (ioctl(fd, TIOCGETP, &sgb) < 0)  {
                perror("Stty_from_defaults");
		EXIT(1);
        }
	if (ioctl(fd, TIOCGLTC, &ltc) < 0)  {
		perror("Stty_from_defaults");
		EXIT(1);
	}
	def_str = defaults_get_string("/Text/Edit_back_char", "\177", (int *)NULL);
	sgb.sg_erase = def_str[0];
	def_str = defaults_get_string("/Text/Edit_back_line", "\025", (int *)NULL);
	sgb.sg_kill = def_str[0];
	def_str = defaults_get_string("/Text/Edit_back_word", "\027", (int *)NULL);
	ltc.t_werasc = def_str[0];
        if (ioctl(fd, TIOCSETP, &sgb) < 0)  {
                perror("Stty_from_defaults");
		EXIT(1);
        }
	if (ioctl(fd, TIOCSLTC, &ltc) < 0)  {
		perror("Stty_from_defaults");
		EXIT(1);
	}
	(void)close(fd);
	EXIT(0);
}

