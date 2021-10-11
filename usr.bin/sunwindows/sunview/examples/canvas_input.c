/*	@(#)canvas_input.c 1.1 92/07/30 SMI	*/
#include <suntool/sunview.h>
#include <suntool/canvas.h>

static void my_event_proc();

main(argc, argv)
int    argc;
char **argv;
{
    Frame frame;

    frame = window_create(NULL, FRAME, 0);
    window_create(frame, CANVAS,
        WIN_CONSUME_KBD_EVENT, WIN_ASCII_EVENTS,
        WIN_EVENT_PROC,        my_event_proc,
        0);
    window_main_loop(frame);
    exit(0);
    /* NOTREACHED */
}

static void
my_event_proc(canvas, event)
    Canvas  canvas;
    Event  *event;
{
   char *string = NULL;

   switch (event_id(event)) {
      case '0':
	string = "zero";
	break;

      case '1':
	string = "one ";
	break;

      case '2':
	string = "two ";
	break;
      
      default: 
	break;
   }
   if (string != NULL)
       pw_text(canvas_pixwin(canvas), 
	       10, 10, PIX_SRC, NULL, string);
}
