/*	@(#)fv.colormap.h 1.1 92/07/30 SMI	*/
/*
* Copyright (c) 1988 by Sun Microsystems, Inc.
*/
#define CMS_FVCOLORMAP            "fvcolormap"
#define CMS_FVCOLORMAPSIZE 8

#define cms_fvcolormapsetup(r,g,b) \
         (r)[0] = 255; (g)[0] = 255; (b)[0] = 255; \
         (r)[1] = 0;   (g)[1] = 0;  (b)[1] = 0; \
         (r)[2] = 32;   (g)[2] = 173; (b)[2] = 0; \
         (r)[3] = 173;   (g)[3] = 0; (b)[3] = 15; \
         (r)[4] = 255; (g)[4] = 183;  (b)[4] = 0; \
         (r)[5] = 4; (g)[5] = 0; (b)[5] = 255; \
         (r)[6] = 255; (g)[6] = 42;  (b)[6] = 9; \
         (r)[7] = 0;   (g)[7] = 0;   (b)[7] = 0; 
