#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)showmap.c 1.1 92/07/30 SMI";
#endif

#include <usercore.h>
#include "demolib.h"

float red[2] = { 0., 0.};
float green[2] = { 0.2, 0.8};
float blue[2] = { 0.7, 0.};

#define	NMAPS	10
#define	MAPDIR	"MAPS"

main(argc, argv) 
int	argc; 
char	*argv[];
{
	int quick_flag;
	char file[32];
	int i;

	get_view_surface(our_surface, argv);
	if (initialize_core(BASIC, SYNCHRONOUS, TWOD)) exit(0);
	initialize_device(KEYBOARD, 1);
	if (initialize_view_surface( our_surface, FALSE)) exit(1);
	select_view_surface( our_surface);
	define_color_indices( our_surface, 0,1,red,green,blue);
	quick_flag=quick_test(argc,argv);
	do {
		for (i = 1; i <= NMAPS; i++) {
			sprintf(file, "%s/map.%d", MAPDIR, i);
			restore_segment(1, file);
			sleep(quick_flag ? 1 : 3);
			delete_all_retained_segments();
		}
	} while(!quick_flag);

	deselect_view_surface(our_surface);
	terminate_core();
	return 0;
}

int quick_test(argc,argv) int argc; char *argv[];
	{
	while (--argc > 0) {
		if(!strncmp(argv[argc],"-q",2))
			return(TRUE);
		}
	return(FALSE);
	}
