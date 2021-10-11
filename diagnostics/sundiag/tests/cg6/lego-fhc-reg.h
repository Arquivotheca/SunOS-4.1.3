
/*
 * FBC Hardware Configuration registers.
 *
 * Hardware register offsets from base address. These offsets are
 * intended to be added to a pointer-to-integer whose value is the
 * base address of the Lego memory mapped register area.
 */

#ifndef lint
static char	sccsid_lego_fhc_reg_h[] = { "@(#)lego-fhc-reg.h 3.2 88/08/23" };
#endif lint

#define L_FHC_CONFIG		( 0x000 / sizeof(int) )


/*
 * FHC CONFIG register bits
 */
typedef enum {
	L_FHC_CONFIG_RES_1024, L_FHC_CONFIG_RES_1152, L_FHC_CONFIG_RES_1280,
	L_FHC_CONFIG_RES_1600
	} l_fhc_config_res_t;

typedef enum {
	L_FHC_CONFIG_CPU_SPARC, L_FHC_CONFIG_CPU_68020, L_FHC_CONFIG_CPU_80386,
	L_FHC_CONFIG_CPU_80386_1
	} l_fhc_config_cpu_t;

struct l_fhc_config {
	unsigned	l_fhc_config_fb_id : 8;		/* frame buffer ID # */
	unsigned	l_fhc_config_version : 4;	/* chip version #   */
	unsigned	l_fhc_config_frop : 1;		/* fast rop enable  */
	unsigned	l_fhc_config_row : 1;		/* row cache enable */
	unsigned	: 1;				/* not used */
	unsigned	l_fhc_config_dst : 1;		/* dst cache enable */
	unsigned	l_fhc_config_reset : 1;		/* 1 == reset	    */
	unsigned	: 2;				/* not used */
	l_fhc_config_res_t	l_fhc_config_resolution : 2;/* resolution   */
	l_fhc_config_cpu_t	l_fhc_config_cpu : 2;	/* host cpu type  */
	unsigned	l_fhc_config_tests_mod : 1;	/* modify tests     */
	unsigned	l_fhc_config_testy : 4;
	unsigned	l_fhc_config_testx : 4;
	};

/*
 * define FHC registers as a structure.
 */
struct l_fhc {
	struct l_fhc_config	l_fhc_config;
	};
