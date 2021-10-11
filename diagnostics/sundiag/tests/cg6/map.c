static  char sccsid[] = "@(#)map.c 1.1 92/07/30 SMI";
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "cg6reg.h"

static	char *fbctec = 0;
static	char *fhcthc = 0;
extern char *device_name;

extern char *getenv();

int *
map_lego_rom()
{
	int	fd, prot, flags;
	int	offset;
	char	*addr;
	char	*device;

	device = device_name;
	offset = 0;

	prot = PROT_READ|PROT_WRITE;
	flags = MAP_SHARED;

	fd = open ( device, O_RDWR );
	addr = (char *)mmap ( 0, CG6_ROM_SZ, prot, flags, fd, offset );
	close ( fd );
	return ( (int *)addr );
}

int *
map_lego_alt()
{
        int     fd, prot, flags;
        int     offset;
        char    *addr;
        char    *device;

        device = device_name;
        offset = 0x280000;
        flags = MAP_PRIVATE;

/*
*        device = getenv ("CG6DEVICE");
*        if ( device == 0 ) device = strdup ( "/dev/sbus1" );
*        offset = 0x280000;
*
*        prot = PROT_READ|PROT_WRITE;
*        flags = MAP_SHARED;
*/
        fd = open ( device, O_RDWR );
        addr = (char *)mmap ( 0, CG6_ROM_SZ, prot, flags, fd, offset );
        close ( fd );
        return ( (int *)addr );
}


int *
map_lego_dac()
{
	int	fd, prot, flags;
	int	offset;
	char	*addr;
	char	*device;

	device = device_name;
	offset = CG6_VADDR_CMAP;
	flags = MAP_PRIVATE;

	if ( strncmp ( device, "/dev/cgsix", 10 ) != 0 ) {
		offset = CG6_ADDR_CMAP;
		flags = MAP_SHARED;
	}

	prot = PROT_READ|PROT_WRITE;

	fd = open ( device, O_RDWR );
	addr = (char *)mmap ( 0, CG6_CMAP_SZ, prot, flags, fd, offset );
	close (fd);
	return ( (int *)addr );
}

int *
map_lego_tec()
{
	int	fd, prot, flags;
	int	offset;
	char	*addr;
	char	*device;

	device = device_name;
	offset = CG6_VADDR_FBC;
	flags = MAP_PRIVATE;

	if ( strncmp ( device, "/dev/cgsix", 10 ) != 0 ) {
		offset = CG6_ADDR_FBC;
		flags = MAP_SHARED;
	}

	prot = PROT_READ|PROT_WRITE;

	fd = open ( device, O_RDWR );
	if ( fbctec == 0 ) {
		addr = (char *)mmap(0, CG6_FBCTEC_SZ, prot, flags, fd, offset);
		fbctec = addr;
	} else addr = fbctec;
	addr += CG6_TEC_POFF;
	close (fd);
	return ( (int *)addr );
}

int *
map_lego_thc()
{
	int	fd, prot, flags;
	int	offset;
	char	*addr;
	char	*device;

	device = device_name;
	offset = CG6_VADDR_FHC;
	flags = MAP_PRIVATE;

	if ( strncmp ( device, "/dev/cgsix", 10 ) != 0 ) {
		offset = CG6_ADDR_FHC;
		flags = MAP_SHARED;
	}

	prot = PROT_READ|PROT_WRITE;

	fd = open ( device, O_RDWR );
	if ( fhcthc == 0 ) {
		addr = (char *)mmap(0, CG6_FHCTHC_SZ, prot, flags, fd, offset);
		fhcthc = addr;
	} else addr = fhcthc;
	addr += CG6_THC_POFF;
	close (fd);
	return ( (int *)addr );
}

int *
map_lego_fbc()
{
	int	fd, prot, flags;
	int	offset;
	char	*addr;
	char	*device;

	device = device_name;
	offset = CG6_VADDR_FBC;
	flags = MAP_PRIVATE;

	if ( strncmp ( device, "/dev/cgsix", 10 ) != 0 ) {
		offset = CG6_ADDR_FBC;
		flags = MAP_SHARED;
	}

	prot = PROT_READ|PROT_WRITE;

	fd = open ( device, O_RDWR );
	if ( fbctec == 0 ) {
		addr = (char *)mmap(0, CG6_FBCTEC_SZ, prot, flags, fd, offset);
		fbctec = addr;
	} else addr = fbctec;
	close (fd);
	return ( (int *)addr );
}

int *
map_lego_fhc()
{
	int	fd, prot, flags;
	int	offset;
	char	*addr;
	char	*device;

	device = device_name;
	offset = CG6_VADDR_FHC;
	flags = MAP_PRIVATE;

	if ( strncmp ( device, "/dev/cgsix", 10 ) != 0 ) {
		offset = CG6_ADDR_FHC;
		flags = MAP_SHARED;
	}

	prot = PROT_READ|PROT_WRITE;

	fd = open ( device, O_RDWR );
	if ( fhcthc == 0 ) {
		addr = (char *)mmap(0, CG6_FHCTHC_SZ, prot, flags, fd, offset);
		fhcthc = addr;
	} else addr = fhcthc;
	close (fd);
	return ( (int *)addr );
}

int *
map_lego_dfb()
{
	int	fd, prot, flags;
	int	offset;
	char	*addr;
	char	*device;

	device = device_name;
	offset = CG6_VADDR_COLOR;
	flags = MAP_PRIVATE;

	if (strncmp (device, "/dev/cgsix", 10 ) != 0 ) {
		offset = CG6_ADDR_COLOR;
		flags = MAP_SHARED;
	}

	prot = PROT_READ|PROT_WRITE;

	fd = open ( device, O_RDWR );
/*
*	addr = (char *)mmap ( 0, CG6_FB_SZ, prot, flags, fd, offset );
*/
	addr = (char *)mmap ( 0, 0x800000, prot, flags, fd, offset );
	close (fd);
	return ( (int *)addr );
}
