#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>

main()
{
    struct pixrect *screen_pixrect;
    struct pixrect *source_pixrect;

    screen_pixrect = pr_open("/dev/cg0");
    source_pixrect = mem_create(640,480,8);
    pr_rop(screen_pixrect,0,0,640,480,PIX_SRC,source_pixrect,0,0);
}
