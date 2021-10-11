/*
 *	@(#)audioio.h 1.1 92/07/30 SMI
 *	Copyright 1989 (c) Sun Microsystems, Inc.
 */

/*  These are the ioctl calls for the SPARCstation 1 audio device.  This
 *  interface is preliminary and subject to change in future releases.
 *  You are encouraged to design your code in a modular fashion so that
 *  future changes to the interface can be incorporated with little trouble.
 */

/*  This is the generalized structure for accessing the audio device registers.
 *
 *  Control specifies the register to which we are writing, plus the length
 *  of the register, in bytes.  The two values are combined to make it easier
 *  to use.  The register is in the high byte, and the length is in the
 *  low byte.  Macros in sbusdev/audioreg.h make it easier to create the
 *  values for the control member.
 *
 *  By setting the register value appropriately, multiple registers can
 *  be loaded with one operation.  The maximum number of bytes loaded at
 *  one time is 46, when all the MAP registers are set.
 */

struct	audio_ioctl {
	short	control;
	unsigned char	data[46];
};


/*  Ioctl calls for the device.
 *
 *  GETREG and SETREG are used to read and write the chip's registers,
 *  using the audio_ioctl structure to pass data back and forth.
*/

#define AUDIOGETREG		_IOWR(i,1,struct audio_ioctl)
#define AUDIOSETREG		_IOW(i,2,struct audio_ioctl)

/*  READSTART tells the device driver to start reading sound.  This is
 *  useful for starting recordings when you don't want to call read()
 *  until later.  STOP stops all i/o and clears the buffers, while
 *  PAUSE stops i/o without clearing the buffers.  RESUME resumes i/o
 *  after a PAUSE.  These ioctl's don't transfer any data.
 */
#define AUDIOREADSTART		_IO(1,3)
#define AUDIOSTOP		_IO(1,4)
#define AUDIOPAUSE		_IO(1,5)
#define AUDIORESUME		_IO(1,6)

/*  READQ is the number of bytes that have been read but not passed to
 *  the application.  WRITEQ is the number of bytes passed into
 *  the driver but not written to the device.  QSIZE is the number of bytes
 *  in the queue.
 */
#define AUDIOREADQ		_IOR(1,7,int)
#define AUDIOWRITEQ		_IOR(1,8,int)
#define AUDIOGETQSIZE		_IOR(1,9,int)
#define AUDIOSETQSIZE		_IOW(1,10,int)
