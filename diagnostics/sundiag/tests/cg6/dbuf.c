/*	dbuf.c	For doublebuffer testing of Quadro family Lego's.
*/

#ifndef lint
static  char sccsid[] = "@(#)dbuf.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include "lego.h"
#include "macros.h"
#include <stdio.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <sgtty.h>
#include <sun/fbio.h>
#include <sys/types.h>
#include <pixrect/pixrect_hs.h>
#include "cg6var.h"
#include <sunwindow/win_cursor.h>
#include <suntool/sunview.h>
#include <suntool/gfx_hs.h>

extern width, height, dblflag, testing_secondary_buffer;
extern Pixrect *prfd;
/*extern *fbc, *fhc, *thc, *dac; */

/*#include <pixrect/pixrect_hs.h> */

#define L_FHC_CONFIG             ( 0x000 / sizeof(int) )

int 	*fbc;
/*	int 	*fhc_base;		*/
int 	*fhc;
int 	*thc;
int 	*dac;


unsigned char buff0red[256], buff0grn[256], buff0blu[256];
unsigned char buff1red[256], buff1grn[256], buff1blu[256];
extern red1[256], grn1[256], blu1[256];

dbuf()

{
	long	tmp;
        int     i, temp;
	int  bc, fc;

	fhc = map_lego_fhc() ;
        temp = read_lego_fhc ( fhc + L_FHC_CONFIG );
/*	printf("\ntemp: %lx\n", temp);
*	printf("\ndblflag: %d", dblflag);
*/

	if (setup()==-1) {
		printf("\nResolution not supported for double buffering.");
		return( 0 ) ;
	}
	bc = read_lego_fbc( fbc + L_FBC_FCOLOR);
	fc = read_lego_fbc( fbc + L_FBC_BCOLOR);

/*	Set DB1 Mode */
	tmp = read_lego_thc ( thc + L_THC_HCMISC );
	tmp &= ~0x300000;
	tmp |= 0x100000;
	write_lego_thc ( thc + L_THC_HCMISC, tmp );

/*	Set Double Buffered Window */
	tmp = read_lego_fbc ( fbc + L_FBC_MISC );
	tmp &= ~0xc00000;
	tmp |= 0x400000;
	write_lego_fbc ( fbc + L_FBC_MISC, tmp );

	display_buff_0();
	write_buff_0_1();
	read_buff_0();

	set_fcolor(255);
	set_bcolor(155);
	set_fcolor(200);
	
	testing_secondary_buffer= TRUE;
	write_buff_1();
	set_fcolor(30);

	testframebuffer();  

	triangle ( 100, 100, 150, height -50, 50, height -50 );
	write_buff_0();
	set_fcolor(180);
	rectangle ( 0, 0, width/3, height/2 );
	rectangle ( width/2+300, height/2+100, width, height );

	sleep(3);
	flip_buffer();
	sleep(3);
	flip_buffer();
	sleep(3);

	for(i=0;i<100;i++)
	flip_buffer();
	set_fcolor(255);
	set_bcolor(0);

	use_buffer_1();
	testing_secondary_buffer=TRUE;
        clear_screen();
	testframebuffer();
	testfb_to_fb();
	test_blits();
        test_lines();
        test_polygons();
        init_cmapnfb( red1, grn1, blu1);
        testcolormap();
        clear_screen();
        test_sine();
	
/*      Reset DB1 Mode */
        tmp = read_lego_thc ( thc + L_THC_HCMISC );
        tmp &= ~0x300000;
        write_lego_thc ( thc + L_THC_HCMISC, tmp );

/*      Set Double Buffered Window */
        tmp = read_lego_fbc ( fbc + L_FBC_MISC );
        tmp &= ~0xc00000;
	set_fcolor(fc);
	set_bcolor(bc);
        write_lego_fbc ( fbc + L_FBC_MISC, tmp );
        write_lego_fhc(fhc + L_FHC_CONFIG,temp&~0x200);

	use_buffer_0();
	testing_secondary_buffer=FALSE;
	
}


setup()
{
        int     count;
	int  config;

	fbc = map_lego_fbc() ;
 /* printf("\n *fhc: %x", *(fhc+ 0x0/(sizeof (int)) ) );  */
	dac = map_lego_dac() ;
	thc = map_lego_thc() ;

	config= read_lego_fhc(fhc+L_FHC_CONFIG);

 if (width  ==1280)
	write_lego_fhc(fhc + L_FHC_CONFIG,config |0x11bb);
 else if (width == 1152)
        write_lego_fhc(fhc + L_FHC_CONFIG,config |0xbbb);
 else if (width == 1024)
        write_lego_fhc(fhc + L_FHC_CONFIG,config |0x1bb);
 else return(-1);
 

        /*write_lego_fbc(fbc + L_FBC_MISC      ,0x0012aa40); */
        write_lego_fbc(fbc + L_FBC_RASTEROFFX,         0);
        write_lego_fbc(fbc + L_FBC_RASTEROFFY,         0);
        write_lego_fbc(fbc + L_FBC_AUTOINCX  ,         0);
        write_lego_fbc(fbc + L_FBC_AUTOINCY  ,         0);
        write_lego_fbc(fbc + L_FBC_CLIPMINX  ,         0);
        write_lego_fbc(fbc + L_FBC_CLIPMINY  ,         0);
        write_lego_fbc(fbc + L_FBC_CLIPMAXX  ,      width);
        write_lego_fbc(fbc + L_FBC_CLIPMAXY  ,       height);
        write_lego_fbc(fbc + L_FBC_CLIPCHECK ,0x00000000);
        write_lego_fbc(fbc + L_FBC_FCOLOR    ,0x000000ff);
        write_lego_fbc(fbc + L_FBC_BCOLOR    ,0x00000000);
        write_lego_fbc(fbc + L_FBC_RASTEROP  ,0xa980cc00);
        write_lego_fbc(fbc + L_FBC_PLANEMASK ,0x000000ff);
        write_lego_fbc(fbc + L_FBC_PIXELMASK ,0xffffffff);
        write_lego_fbc(fbc + L_FBC_PATTALIGN ,0x00000000);
        write_lego_fbc(fbc + L_FBC_PATTERN0  ,0xffffffff);
        write_lego_fbc(fbc + L_FBC_PATTERN1  ,0xffffffff);
        write_lego_fbc(fbc + L_FBC_PATTERN2  ,0xffffffff);
        write_lego_fbc(fbc + L_FBC_PATTERN3  ,0xffffffff);
        write_lego_fbc(fbc + L_FBC_PATTERN4  ,0xffffffff);
        write_lego_fbc(fbc + L_FBC_PATTERN5  ,0xffffffff);
        write_lego_fbc(fbc + L_FBC_PATTERN6  ,0xffffffff);
        write_lego_fbc(fbc + L_FBC_PATTERN7  ,0xffffffff);
	return(0);
}

write_buff_0_1()

{
        long    tmp;

        busy();
        tmp = read_lego_fbc ( fbc + L_FBC_MISC );
        tmp &= ~0xf00;

        tmp |= 0x500;
        write_lego_fbc ( fbc + L_FBC_MISC, tmp );
}
write_buff_0()

{
        long    tmp;

        busy();
        tmp = read_lego_fbc ( fbc + L_FBC_MISC );
        tmp &= ~0xf00;

        tmp |= 0x600;
        write_lego_fbc ( fbc + L_FBC_MISC, tmp );
}

write_buff_1()

{
        long    tmp;

        busy();
        tmp = read_lego_fbc ( fbc + L_FBC_MISC );
        tmp &= ~0xf00;

        tmp |= 0x900;
        write_lego_fbc ( fbc + L_FBC_MISC, tmp );
}

read_buff_0()

{
        long    tmp;

        busy();
        tmp = read_lego_fbc ( fbc + L_FBC_MISC );
        tmp &= ~0x3000;
        tmp |= 0x1000;
        write_lego_fbc ( fbc + L_FBC_MISC, tmp );
}

read_buff_1()

{
        long    tmp;

        busy();
        tmp = read_lego_fbc ( fbc + L_FBC_MISC );
        tmp &= ~0x3000;
        tmp |= 0x2000;
        write_lego_fbc ( fbc + L_FBC_MISC, tmp );
}

display_buff_1()

{
        long    tmp;
        long    *hcmisc;
        long    *fhcconf;

        busy();
        tmp = read_lego_fhc ( fhc + L_FHC_CONFIG );
        tmp |= 0x200;
        write_lego_fhc ( fhc + L_FHC_CONFIG, tmp );
}

display_buff_0()

{

        long    tmp;

        busy();
        tmp = read_lego_fhc ( fhc + L_FHC_CONFIG );
        tmp &= ~0x200;
        write_lego_fhc ( fhc + L_FHC_CONFIG, tmp );
}

use_buffer_0()
{
	
	read_buff_0();
	write_buff_0();
	display_buff_0();
}
use_buffer_1()
{
	
	read_buff_1();
	write_buff_1();
	display_buff_1();
}
flip_buffer()

{
        long    tmp;

        busy();
	tmp = read_lego_fhc ( fhc+L_FHC_CONFIG);

	if ((tmp & 0x200) == 1)
       {

	  pr_getcolormap(prfd, 0, 256, buff0red, buff0grn, buff0blu);
	  pr_putcolormap(prfd, 0, 256, buff1red, buff1grn, buff1blu);
 	}	
	else
	{
	  pr_getcolormap(prfd, 0, 256, buff1red, buff1grn, buff1blu);
	  pr_getcolormap(prfd, 0, 256, buff0red, buff0grn, buff0blu);

	}

        tmp = read_lego_fbc ( fbc + L_FBC_MISC );
        tmp |= 0x2000000;
        write_lego_fbc ( fbc + L_FBC_MISC, tmp );

        while ( read_lego_fbc ( fbc + L_FBC_MISC ) & 0x1000000 ) ;
}

sh()
{
 printf("\nRect1 - FBC: 0x%x", read_lego_fbc(fbc+L_FBC_MISC));
 printf("\nFHC: 0x%x", read_lego_fhc(fhc+L_FHC_CONFIG));
 printf("\nTHC: 0x%x", read_lego_thc(thc+L_THC_HCMISC));
}

