#define	VERSION		"(c) 1988 Sun Microsystems     wings Version 0.003"

#define	NO_ERROR		0

#define	TRUE			1
#define	FALSE			0


/* this strucure keeps up to date information about your
   plane as well as the enemies */
#define	STRUCT_STATS								\
	float			pos_x;		/* latitude		*/	\
	float			pos_y;		/* longitude		*/	\
	float			pos_z;		/* altitude		*/	\
	float			pos_dx;		/* change in latitude	*/	\
	float			pos_dy;		/* change in longitude	*/	\
	float			pos_dz;		/* change in altitude	*/	\
	int			rot_x;		/* pitch		*/	\
	int			rot_y;		/* yaw			*/	\
	int			rot_z;		/* roll			*/	\
	int			rot_dx;		/* change in pitch	*/	\
	int			rot_dy;		/* change in yaw	*/	\
	int			rot_dz;		/* change in roll	*/





/* the size of this struct can not exceed 4K bytes */
struct shared_buffer {

	/*************************************\
	 * The following attributes are ONLY *
	 *     set by the RENDER process     *
	\*************************************/
	unsigned int		drawing;	/* Only frame process reads   */

	STRUCT_STATS				/* Only network process reads */


	/*************************************\
	 * The following attributes are ONLY *
	 *      set by the FRAME process     *
	\*************************************/
	unsigned int		ok_to_draw,	/* Only render process reads  */
				window_minx,
				window_miny,
				window_maxx,
				window_maxy,
				mouse_rel_x,
				mouse_rel_y;

	unsigned int		enet_type;	/* Only network process reads */


	/*************************************\
	 * The following attributes are ONLY *
	 *     set by the NETWORK process    *
	\*************************************/
	struct stats	{ STRUCT_STATS }  enemies[16];    /* Only render process reads  */
} ;

extern struct shared_buffer *Shmem;
