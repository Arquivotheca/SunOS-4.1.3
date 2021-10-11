#define set_lego_clip(x,y,x2,y2)        \
	busy();	\
        write_lego_fbc( fbc + L_FBC_CLIPMINX , (x)); \
        write_lego_fbc( fbc + L_FBC_CLIPMINY , (y)); \
        write_lego_fbc( fbc + L_FBC_CLIPMAXX , (x2) - 1); \
        write_lego_fbc( fbc + L_FBC_CLIPMAXY , (y2) - 1)

#define set_rasteroff(x,y)      \
	busy();	\
        write_lego_fbc( fbc + L_FBC_RASTEROFFX , (x)); \
        write_lego_fbc( fbc + L_FBC_RASTEROFFY , (y))

#define draw() \
        while ( read_lego_fbc( fbc + L_FBC_DRAWSTATUS) & L_FBC_FULL );

#define blitwait() \
        while ( read_lego_fbc( fbc + L_FBC_BLITSTATUS) & L_FBC_FULL );

#define busy() \
        while ( read_lego_fbc( fbc + L_FBC_STATUS) & L_FBC_BUSY );

#define set_fcolor(val) \
        write_lego_fbc( fbc + L_FBC_FCOLOR , (val))

#define set_bcolor(val) \
        write_lego_fbc( fbc + L_FBC_BCOLOR , (val))

#define triangle(x1,y1,x2,y2,x3,y3) \
	busy();	\
        write_lego_fbc( fbc + L_FBC_ITRIABSY , (y1));     \
        write_lego_fbc( fbc + L_FBC_ITRIABSX , (x1));     \
        write_lego_fbc( fbc + L_FBC_ITRIABSY , (y2));     \
        write_lego_fbc( fbc + L_FBC_ITRIABSX , (x2));     \
        write_lego_fbc( fbc + L_FBC_ITRIABSY , (y3));     \
        write_lego_fbc( fbc + L_FBC_ITRIABSX , (x3));     \
        draw()

#define line(x1,y1,x2,y2)       \
	busy();	\
        write_lego_fbc( fbc + L_FBC_ILINEABSY , (y1)); \
        write_lego_fbc( fbc + L_FBC_ILINEABSX , (x1)); \
        write_lego_fbc( fbc + L_FBC_ILINEABSY , (y2)); \
        write_lego_fbc( fbc + L_FBC_ILINEABSX , (x2)); \
        draw()
 
#define point(x,y)      \
	busy();	\
        write_lego_fbc( fbc + L_FBC_IPOINTABSY , (y));    \
        write_lego_fbc( fbc + L_FBC_IPOINTABSX , (x));    \
        draw()
 
#define rectangle(x1,y1,x2,y2)  \
	busy();	\
        write_lego_fbc( fbc + L_FBC_IRECTABSY , (y1)); \
        write_lego_fbc( fbc + L_FBC_IRECTABSX , (x1)); \
        write_lego_fbc( fbc + L_FBC_IRECTABSY , (y2)); \
        write_lego_fbc( fbc + L_FBC_IRECTABSX , (x2)); \
        draw()

#define set_color(offset,r,g,b) \
	write_lego_dac ( dac, (offset)<<24 );	\
	write_lego_dac ( dac + 1, (r)<<24 );	\
	write_lego_dac ( dac + 1, (g)<<24 );	\
	write_lego_dac ( dac + 1, (b)<<24 )

#define blit(sx1,sy1,sx2,sy2,dx1,dy1,dx2,dy2)	\
	busy();	\
	write_lego_fbc( fbc + L_FBC_X0,(sx1) ); \
	write_lego_fbc( fbc + L_FBC_Y0,(sy1) ); \
	write_lego_fbc( fbc + L_FBC_X1,(sx2) ); \
	write_lego_fbc( fbc + L_FBC_Y1,(sy2) ); \
	write_lego_fbc( fbc + L_FBC_X2,(dx1) ); \
	write_lego_fbc( fbc + L_FBC_Y2,(dy1) ); \
	write_lego_fbc( fbc + L_FBC_X3,(dx2) ); \
	write_lego_fbc( fbc + L_FBC_Y3,(dy2) ); \
	blitwait();
