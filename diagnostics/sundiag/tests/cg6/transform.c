static char version[] = "Version 1.1";
static char     sccsid[] = "@(#)transform.c 1.1 7/30/92 Copyright 1988 Sun Micro";

/* Rendering routine for wings by Curtis Priem, Oct 1988 */

#include "render_main.h"

float sine[SINE_SIZE] = {
#include "create_sine.h"
};

float cosine[COSINE_SIZE] = {
#include "create_cosine.h"
};

/* #define  DELAYLOOP	{int iii; for(iii=0; iii<50; iii++);} */
#define DELAYLOOP	{}

int sinehgt;	/* set before call to build_view_matrix */

build_view_matrix()
{
  register int    *tec_base_ptr;
  register int    zero, one;
  float  fzero=0.0, fone=1.0;

  zero = *(int*)&fzero;
  one  = *(int*)&fone;

  tec_base_ptr = tec_base;
  write_lego_tec(tec_base_ptr+L_TEC_CLIPCHECK,   0x00000000);
  write_lego_tec(tec_base_ptr+L_TEC_VDC_MATRIX,  0x00004000);


sinehgt += 0x0045;
  /***** load translation matrix *****/
  {
    register int    tranx, trany, tranz;
float fheight, ftmpz;
fheight = cosine[(sinehgt & 0xffff)>>(16-COSINE_POWER)];
    tranx = *(int*)&(Shmem->pos_x) ^ 0x80000000;
    trany = *(int*)&(Shmem->pos_y) ^ 0x80000000;
    tranz = *(int*)&(Shmem->pos_z) ^ 0x80000000;
ftmpz = Shmem->pos_z * fheight / -2.0;
    write_lego_tec(tec_base_ptr+L_TEC_DATA48,  one  );
    write_lego_tec(tec_base_ptr+L_TEC_DATA49,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA50,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA51,  tranx);
    write_lego_tec(tec_base_ptr+L_TEC_DATA52,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA53,  one  );
    write_lego_tec(tec_base_ptr+L_TEC_DATA54,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA55,  trany);
    write_lego_tec(tec_base_ptr+L_TEC_DATA56,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA57,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA58,  *(int*)&fheight);
    write_lego_tec(tec_base_ptr+L_TEC_DATA59,  *(int*)&ftmpz);
    write_lego_tec(tec_base_ptr+L_TEC_DATA60,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA61,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA62,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA63,  one  );
  }



  /***** load rotation x matrix *****/
  {
    register int    sinx, cosx, msinx; 
    sinx  = *(int*)&sine[(Shmem->rot_x & 0xffff)>>(16-SINE_POWER)]; 
    msinx = sinx ^ 0x80000000; 
    cosx  = *(int*)&cosine[(Shmem->rot_x & 0xffff)>>(16-COSINE_POWER)];
    write_lego_tec(tec_base_ptr+L_TEC_DATA32,  one  );
    write_lego_tec(tec_base_ptr+L_TEC_DATA33,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA34,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA35,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA36,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA37,  cosx );
    write_lego_tec(tec_base_ptr+L_TEC_DATA38,  sinx );
    write_lego_tec(tec_base_ptr+L_TEC_DATA39,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA40,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA41,  msinx);
    write_lego_tec(tec_base_ptr+L_TEC_DATA42,  cosx );
    write_lego_tec(tec_base_ptr+L_TEC_DATA43,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA44,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA45,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA46,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA47,  one  );
  }



  /***** concatenate *****/
  write_lego_tec(tec_base_ptr+L_TEC_COMMAND4,  0xf0302010);
  write_lego_tec(tec_base_ptr+L_TEC_COMMAND4,  0xf0312011);
DELAYLOOP
  write_lego_tec(tec_base_ptr+L_TEC_COMMAND4,  0xf0322012);
DELAYLOOP
  write_lego_tec(tec_base_ptr+L_TEC_COMMAND4,  0xf0332013);
DELAYLOOP



  /***** load rotation y matrix *****/
  {
    register int    siny, cosy, msiny; 
    siny  = *(int*)&sine[(Shmem->rot_y & 0xffff)>>(16-SINE_POWER)]; 
    msiny = siny ^ 0x80000000; 
    cosy  = *(int*)&cosine[(Shmem->rot_y & 0xffff)>>(16-COSINE_POWER)];
    write_lego_tec(tec_base_ptr+L_TEC_DATA32,  cosy );
    write_lego_tec(tec_base_ptr+L_TEC_DATA33,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA34,  msiny);
    write_lego_tec(tec_base_ptr+L_TEC_DATA35,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA36,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA37,  one  );
    write_lego_tec(tec_base_ptr+L_TEC_DATA38,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA39,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA40,  siny );
    write_lego_tec(tec_base_ptr+L_TEC_DATA41,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA42,  cosy );
    write_lego_tec(tec_base_ptr+L_TEC_DATA43,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA44,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA45,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA46,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA47,  one  );
  }



  /***** concatenate *****/
  write_lego_tec(tec_base_ptr+L_TEC_COMMAND4,  0xf0102030);
  write_lego_tec(tec_base_ptr+L_TEC_COMMAND4,  0xf0112031);
DELAYLOOP
  write_lego_tec(tec_base_ptr+L_TEC_COMMAND4,  0xf0122032);
DELAYLOOP
  write_lego_tec(tec_base_ptr+L_TEC_COMMAND4,  0xf0132033);
DELAYLOOP



  /***** load rotation z matrix *****/
  {
    register int    sinz, cosz, msinz; 
    sinz  = *(int*)&sine[(Shmem->rot_z & 0xffff)>>(16-SINE_POWER)]; 
    msinz = sinz ^ 0x80000000; 
    cosz  = *(int*)&cosine[(Shmem->rot_z & 0xffff)>>(16-COSINE_POWER)];
    write_lego_tec(tec_base_ptr+L_TEC_DATA32,  cosz );
    write_lego_tec(tec_base_ptr+L_TEC_DATA33,  sinz );
    write_lego_tec(tec_base_ptr+L_TEC_DATA34,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA35,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA36,  msinz);
    write_lego_tec(tec_base_ptr+L_TEC_DATA37,  cosz );
    write_lego_tec(tec_base_ptr+L_TEC_DATA38,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA39,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA40,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA41,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA42,  one  );
    write_lego_tec(tec_base_ptr+L_TEC_DATA43,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA44,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA45,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA46,  zero );
    write_lego_tec(tec_base_ptr+L_TEC_DATA47,  one  );
  }



  /***** concatenate *****/
  write_lego_tec(tec_base_ptr+L_TEC_COMMAND4,  0xf0302010);
  write_lego_tec(tec_base_ptr+L_TEC_COMMAND4,  0xf0312011);
DELAYLOOP
  write_lego_tec(tec_base_ptr+L_TEC_COMMAND4,  0xf0322012);
DELAYLOOP
  write_lego_tec(tec_base_ptr+L_TEC_COMMAND4,  0xf0332013);
DELAYLOOP



  /***** load VDC matrix *****/
  {
    float fscalex=0.10, ftranx, fscaley= -0.10, ftrany;
    ftranx = (Shmem->window_minx + Shmem->window_maxx)/2;
    ftrany = (Shmem->window_miny + Shmem->window_maxy)/2;
    write_lego_tec(tec_base_ptr+L_TEC_DATA12,  *(int*)&fscalex);
    write_lego_tec(tec_base_ptr+L_TEC_DATA13,  *(int*)&ftranx );
    write_lego_tec(tec_base_ptr+L_TEC_DATA14,  *(int*)&fscaley);
    write_lego_tec(tec_base_ptr+L_TEC_DATA15,  *(int*)&ftrany );
  }



  {
    register int clipcheck;
    clipcheck = read_lego_tec(tec_base_ptr+L_TEC_CLIPCHECK);
    if(clipcheck < 0)
	errorprint(-25, "build_view_matrix: TEC_EXCEPTION" );
  }

}
