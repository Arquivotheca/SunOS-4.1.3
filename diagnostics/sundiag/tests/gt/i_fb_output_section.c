#ifndef lint
static  char sccsid[] = "@(#)i_fb_output_section.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <fe_includes.h>
#include <hdwr_regs.h>


#define REG(i)	i
#define CLUT_LENGTH	8192
#define CLUT_DATA_MASK	0xffffff
#define DELAY_ZERO	0x1000
#define DELAY_ONE	0x100000
#define TOLERANT	0x1000

/* checksums expected from correct hardware */
#define CHKSUM_BANK_0	0xD50C7143
#define CHKSUM_BANK_1	0xB66ADC9A
#define CHKSUM_BANK_2	0xB2C0B0B9
#define CHKSUM_BANK_3	0xE055C963
#define CHKSUM_BANK_4	0xD5684AAF


/**********************************************************************/
void
i_fb_output_section(ctxp, umcbp, kmcbp, fputs) 
/**********************************************************************/
register Hk_context	*ctxp;		/* r16 = context      pointer */
register Hkvc_umcb	*umcbp;		/* r17 = user mcb     pointer */
register Hkvc_kmcb	*kmcbp;		/* r18 = kernel mcb   pointer */
register void		(*fputs)();	/* r19 = vcom_fputs() pointer */

{

    void shadow2mem();
    void mem2shadow();
    void hexasc();
    void chksum();
    void restore_clk();
    int clock_chk();

    unsigned int expected_sum[5];
    unsigned int sum[5];
    unsigned int *csr;
    unsigned int *addr = (unsigned int *) FBD_CLUT_0;
    int bank;
    int i;
    int res = 0;
    int tmpres = 0;
    int hdwr;
    unsigned int save[CLUT_LENGTH];

    char string[14];
    char nl[2];
    char comma[3];
    char txtdat[18];
    char expect[4];
    char actual[4];
    char banktxt[5];
    char warning[31];

    nl[0] = '\n';
    nl[1] = '\0';

    comma[0] = ',';
    comma[1] = ' ';
    comma[2] = '\0';


    txtdat[0] = 'F';txtdat[1] = 'B';txtdat[2] = 'o';txtdat[3] = 'u';
    txtdat[4] = 't';txtdat[5] = '_';txtdat[6] = 't';txtdat[7] = 'r';
    txtdat[8] = 'a';txtdat[9] = 'c';txtdat[10] = 'e';
    txtdat[11] = '-';txtdat[12] = 'E';txtdat[13] = 'r';txtdat[14] = 'r';
    txtdat[15] = ':';txtdat[16] = '\n';txtdat[17] = '\0';

    expect[0] = 'E'; expect[1] = 'x'; expect[2] = 'p'; expect[3] = '\0';
    actual[0] = 'A'; actual[1] = 'c'; actual[2] = 't'; actual[3] = '\0';
    banktxt[0] = 'B'; banktxt[1] = 'a'; banktxt[2] = 'n';
    banktxt[3] = 'k'; banktxt[4] = '\0';

    warning[0] = 'W';warning[1] = 'A';warning[2] = 'R';
    warning[3] = 'N';warning[4] = 'I';warning[5] = 'N';
    warning[6] = 'G';warning[7] = ':';warning[8] = ' ';
    warning[9] = 'M';warning[10] = 'i';warning[11] = 's';
    warning[12] = 's';warning[13] = 'i';warning[14] = 'n';
    warning[15] = 'g';warning[16] = ' ';warning[17] = 'P';
    warning[18] = 'i';warning[19] = 'x';warning[20] = 'c';
    warning[21] = 'l';warning[22] = 'o';warning[23] = 'c';
    warning[24] = 'k';warning[25] = ' ';warning[26] = 'h';
    warning[27] = '/';warning[28] = 'w';warning[29] = '.';
    warning[30] = '\0';

    /* Skipp the test if the hardware is not there */
    hdwr = clock_chk(fputs);
    if (hdwr == 0) { /* Hardware missing */
	(void) (*fputs) (warning);
	(void) (*fputs) (nl);
	umcbp->errorcode = 0;
	return;
    } else if (hdwr == -1) { /* time out: signal stuck to 0 or 1 */
	umcbp->errorcode = -3;
	return;
    }

    /* save shadow memory */
    (void)shadow2mem(save);
	
    /* turn trace buffer on and generate cheksum for each of the 5 banks */

    for (bank = 0 ; bank < 5 ; bank++) {
#ifdef DEBUG
	res = trace2shadow(bank, fputs);
#else DEBUG
	res = trace2shadow(bank);
#endif DEBUG
	if (res) {
	    goto error_return;
	}

	/* generate check sum */
	sum[bank] = 0;
        addr = (unsigned int *) FBD_CLUT_0;
	for (i = 0 ; i < CLUT_LENGTH/2 ; i++) {
	    chksum(&(sum[bank]), (*(addr++) & 0xffffff));
	}
    }

    /* initialize expected checksums */
    expected_sum[0] =  CHKSUM_BANK_0; expected_sum[1] =  CHKSUM_BANK_1;
    expected_sum[2] =  CHKSUM_BANK_2; expected_sum[3] =  CHKSUM_BANK_3;
    expected_sum[4] =  CHKSUM_BANK_4;

    /* verify checksums */
    for (bank = 0 ; bank < 5 ; bank++) {
	if (sum[bank] != expected_sum[bank]) {
	    if (res == 0)
	       (void) (*fputs) (txtdat);
	    (void) (*fputs) (banktxt);
	    hexasc(bank, string);
	    (void) (*fputs) (string);
	    (void) (*fputs) (comma);
	    (void) (*fputs) (expect);
	    hexasc(expected_sum[bank], string);
	    (void) (*fputs) (string);
	    (void) (*fputs) (comma);
	    (void) (*fputs) (actual);
	    hexasc(sum[bank], string);
	    (void) (*fputs) (string);
	    (void) (*fputs) (nl);

	    res = -4;
	}
    }

error_return:

    /* restore Pix clock */
    restore_clk(hdwr);

    /* restore shadow memory */
    (void)mem2shadow(save);

    /* returning to host */
    umcbp->errorcode = res;
    return;

}

/**********************************************************************/
void
shadow2mem(s)
/**********************************************************************/
unsigned int s[];

{
    register int i;
    register unsigned * volatile addr;

    for (i = 0, addr = (unsigned int *) FBD_CLUT_0 ;
				i < CLUT_LENGTH ; i++, addr++) {
	s[i] = (*addr) & CLUT_DATA_MASK;
    }
}

/**********************************************************************/
void
mem2shadow(s)
/**********************************************************************/
unsigned int s[];

{
    register int i;
    register unsigned * volatile addr;

    for (i = 0, addr = (unsigned int *) FBD_CLUT_0 ;
				i < CLUT_LENGTH ; i++, addr++) {
	*addr = s[i] & CLUT_DATA_MASK;
    }
}


/**********************************************************************/
int
clock_chk(putstr)
/**********************************************************************/
void (*putstr)();
{
    void restore_clk();
    int change_pixclk();
    int measure_vert_retrace();

    int count_before;
    int count_after;
    int clk_sel;

#ifdef DEBUG1
    char	string[14];
    char	nl[2];
    char	comma[3];

    nl[0] = '\n';
    nl[1] = '\0';
    comma[0] = ',';
    comma[1] = ' ';
    comma[2] = '\0';
#endif DEBUG1

    /* measure the length of vertical retrace, BEFORE changing
       the Pixclock
     */
    count_before = measure_vert_retrace();
    if (count_before == -1) { /* timeout for some reason */
	return -1;
    }

#ifdef DEBUG1
    hexasc(count_before, string);
    (void) (*putstr) (string);
    (void) (*putstr) (nl);
#endif DEBUG1

    /* change the pixel clock to half of normal rate */
    clk_sel = change_pixclk();

    /* measure the length of vertical retrace, AFTER changing
       the Pixclock
     */
    count_after = measure_vert_retrace();

#ifdef DEBUG1
    hexasc(count_after, string);
    (void) (*putstr) (string);
    (void) (*putstr) (nl);
#endif DEBUG1

    if (count_after == -1) { /* missing pixclock h/w */
	restore_clk(clk_sel);
	return 0;
    }

    if (count_after < count_before-TOLERANT) {
	return (clk_sel + 2);
    } else {
	restore_clk(clk_sel);
	return 0;
    }
}

/**********************************************************************/
int
change_pixclk()
/**********************************************************************/
{

    register unsigned int *video_mode0 = (unsigned int *)VIDEO_MODE0;
    register unsigned int *video_mode1 = (unsigned int *)VIDEO_MODE1;
    int i, tmp;

    /* turn off sync */
    *video_mode0 &= ~0x1;

    /* delay */
    for (i = 0xfffff ; i ; i--);

    /* change the pix clock */
    tmp = *video_mode1;		/* get crystal selection */
    tmp &= 0x01;
    *video_mode1 |= 0x10;
    *video_mode1 &= ~0x05;	/* select 135 Mhz crystal, video off */

    /* delay */
    for (i = 0xfffff ; i ; i--);

    /* turn on sync */
    *video_mode0 |= 0x1;

    return (tmp);
}

/**********************************************************************/
void
restore_clk(clk_sel)
/**********************************************************************/
int clk_sel;
{
    register unsigned int *video_mode0 = (unsigned int *)VIDEO_MODE0;
    register unsigned int *video_mode1 = (unsigned int *)VIDEO_MODE1;
    int i;

    /* turn off sync */
    *video_mode0 &= ~0x1;

    /* restore pix clock */
    *video_mode1 &= ~0x10;
    /* restore crystal and video on */
    *video_mode1 |= ((clk_sel & 0x01) | 0x04);

    /* delay */
    for (i = 0xfffff ; i ; i--);

    /* turn on sync */
    *video_mode0 |= 0x1;

}

/**********************************************************************/
int
measure_vert_retrace()
/**********************************************************************/
{

    register unsigned int *csr = (unsigned int *)F_PBM_CSR;
    int j = 0;
    int k = 0;

    /* if vertical retrace is 0, wait for it to go to 1 */
    if (!(*csr & 0x2)) {
	/* wait for transition 0 to 1 */
	k = DELAY_ONE;
	while (!(*csr & 0x2) && k--);
	if (k <= 0) { /* time out: stuck to 0 */
	    return -1;
	}
    }

    /* start monitor the 1 untill it becommes 0*/
    j = DELAY_ZERO;
    while ((*csr & 0x2) && j--);
    if (j <= 0) { /* time out: stuck to 1 */
	return -1;
    }

    /* start counting the time, untill 0 becommes 1 */
    k = (DELAY_ONE >> 4) * 3;
    while (!(*csr & 0x2) && k--); /* should not get a timeout here */

    return k;
}


#ifdef DEBUG
/**********************************************************************/
int
trace2shadow(b, putstr)
/**********************************************************************/
int b;
void (*putstr)();
#else !DEBUG
/**********************************************************************/
int
trace2shadow(b)
/**********************************************************************/
int b;
#endif DEBUG

{
    register unsigned int cmd;
    register unsigned int csr0;
    register unsigned int csr1;
    register unsigned int mask;
    register unsigned int *csr = (unsigned int *)F_PBM_CSR;
    register unsigned int *clut = (unsigned int *)FBD_CLUT_CSR0;
    register int j;
    register int k;

#ifdef DEBUG
	char	string[14];
	char    nl[2];
	char	comma[3];

	nl[0] = '\n';
	nl[1] = '\0';
	comma[0] = ',';
	comma[1] = ' ';
	comma[2] = '\0';
#endif DEBUG

    cmd = (b << 4) + 6;
    mask = cmd & 0x3;

    *clut = cmd;

     j = DELAY_ZERO;
     k = DELAY_ONE;
     do {
        csr0 = *clut++;        /* get CSR0 data */
        csr1 = *clut--;        /* get CSR1 data */
	if (*csr & 0x2) {
	    j--;
	} else {
	    k--;
	}
     } while ((csr0 & mask) || (csr1 & mask) && j && k);

     if (j <= 0 || k <= 0) { /* time out */
	return -3;
    }

#ifdef DEBUG
    hexasc(j, string);
    (void) (*putstr) (string);
    (void) (*putstr) (comma);
    hexasc(k, string);
    (void) (*putstr) (string);
    (void) (*putstr) (nl);
#endif DEBUG

    return 0;

}

/**********************************************************************/
void
chksum(sum, data)
/**********************************************************************/
unsigned int *sum;
unsigned int data;

{
    unsigned int oldsum;

    oldsum = *sum;
    *sum += data;
    if (*sum < oldsum) {
	(*sum)++;
    }
}

/**********************************************************************/
void
hexasc(hex, str)
/**********************************************************************/
unsigned int hex;
char *str;
{
    unsigned char c;
    int i;

    str[0] = ' ';
    str[1] = '=';
    str[2] = ' ';
    str[3] = '0';
    str[4] = 'x';
    for (i=0 ; i < 8 ; i++) {
	c = (char)(hex & 0xf);
	str[12-i] = (c < 0xa)? c + '0' : (c-0xa) + 'A';
	hex >>= 4;
    }
    str[i+5] = '\0';
    return;
}
