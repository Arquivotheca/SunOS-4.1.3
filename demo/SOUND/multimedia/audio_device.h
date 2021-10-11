/*	@(#)audio_device.h 1.1 92/07/30 SMI	*/
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

#ifndef _multimedia_audio_io_h
#define	_multimedia_audio_io_h

#include <sun/audioio.h>

typedef audio_info_t	Audio_info;

/*
 * The following macros read the current audio device configuration
 * and convert the data encoding format into an Audio_hdr.
 * 'F' is an open audio file descriptor.
 * 'H' is a pointer to an Audio_hdr.
 * The structure '*H' is updated after the device state has been read.
 */
#define	audio_get_play_config(F, H)					\
		audio__setplayhdr((F), (H), AUDIO__PLAY)
#define	audio_get_record_config(F, H)					\
		audio__setplayhdr((F), (H), AUDIO__RECORD)

/*
 * The following macros attempt to reconfigure the audio device so that
 * it operates on data encoded according to a given Audio_hdr.
 * 'F' is an open audio file descriptor.
 * 'H' is a pointer to an Audio_hdr describing the desired encoding.
 * The structure '*H' is updated after the device state has been read
 * to reflect the actual state of the device.
 *
 * AUDIO_SUCCESS is returned if the device configuration matches the
 * requested encoding.  AUDIO_ERR_NOEFFECT is returned if it does not.
 */
#define	audio_set_play_config(F, H)					\
		audio__setplayhdr((F), (H), AUDIO__SET|AUDIO__PLAY)
#define	audio_set_record_config(F, H)					\
		audio__setplayhdr((F), (H), AUDIO__SET|AUDIO__RECORD)


/*
 * The following macros pause or resume the audio play and/or record channels.
 * Note that requests to pause a channel that is not open will have no effect.
 * In such cases, AUDIO_ERR_NOEFFECT is returned.
 */
#define	audio_pause(F)							\
		audio__setpause((F), AUDIO__PLAYREC|AUDIO__PAUSE)
#define	audio_pause_play(F)						\
		audio__setpause((F), AUDIO__PLAY|AUDIO__PAUSE)
#define	audio_pause_record(F)						\
		audio__setpause((F), AUDIO__RECORD|AUDIO__PAUSE)

#define	audio_resume(F)							\
		audio__setpause((F), AUDIO__PLAYREC|AUDIO__RESUME)
#define	audio_resume_play(F)						\
		audio__setpause((F), AUDIO__PLAY|AUDIO__RESUME)
#define	audio_resume_record(F)						\
		audio__setpause((F), AUDIO__RECORD|AUDIO__RESUME)


/*
 * The following macros get individual state values.
 * 'F' is an open audio file descriptor.
 * 'V' is a pointer to an unsigned int.
 * The value '*V' is updated after the device state has been read.
 */
#define	audio_get_play_port(F, V)					\
		audio__setval((F), (V), AUDIO__PLAY|AUDIO__PORT)
#define	audio_get_record_port(F, V)					\
		audio__setval((F), (V), AUDIO__RECORD|AUDIO__PORT)
#define	audio_get_play_samples(F, V)					\
		audio__setval((F), (V), AUDIO__PLAY|AUDIO__SAMPLES)
#define	audio_get_record_samples(F, V)					\
		audio__setval((F), (V), AUDIO__RECORD|AUDIO__SAMPLES)
#define	audio_get_play_error(F, V)					\
		audio__setval((F), (V), AUDIO__PLAY|AUDIO__ERROR)
#define	audio_get_record_error(F, V)					\
		audio__setval((F), (V), AUDIO__RECORD|AUDIO__ERROR)
#define	audio_get_play_eof(F, V)					\
		audio__setval((F), (V), AUDIO__PLAY|AUDIO__EOF)

#define	audio_get_play_open(F, V)					\
		audio__setval((F), (V), AUDIO__PLAY|AUDIO__OPEN)
#define	audio_get_record_open(F, V)					\
		audio__setval((F), (V), AUDIO__RECORD|AUDIO__OPEN)
#define	audio_get_play_active(F, V)					\
		audio__setval((F), (V), AUDIO__PLAY|AUDIO__ACTIVE)
#define	audio_get_record_active(F, V)					\
		audio__setval((F), (V), AUDIO__RECORD|AUDIO__ACTIVE)
#define	audio_get_play_waiting(F, V)					\
		audio__setval((F), (V), AUDIO__PLAY|AUDIO__WAITING)
#define	audio_get_record_waiting(F, V)					\
		audio__setval((F), (V), AUDIO__RECORD|AUDIO__WAITING)


/*
 * The following macros set individual state values.
 * 'F' is an open audio file descriptor.
 * 'V' is a pointer to an unsigned int.
 * The value '*V' is updated after the device state has been read.
 */
#define	audio_set_play_port(F, V)					\
		audio__setval((F), (V), AUDIO__SET|AUDIO__PLAY|AUDIO__PORT)
#define	audio_set_record_port(F, V)					\
		audio__setval((F), (V), AUDIO__SET|AUDIO__RECORD|AUDIO__PORT)

/*
 * The value returned for these is the value *before* the state was changed.
 * This allows you to atomically read and reset their values.
 */
#define	audio_set_play_samples(F, V)					\
		audio__setval((F), (V), AUDIO__SET|AUDIO__PLAY|AUDIO__SAMPLES)
#define	audio_set_record_samples(F, V)					\
		audio__setval((F), (V), AUDIO__SET|AUDIO__RECORD|AUDIO__SAMPLES)
#define	audio_set_play_error(F, V)					\
		audio__setval((F), (V), AUDIO__SET|AUDIO__PLAY|AUDIO__ERROR)
#define	audio_set_record_error(F, V)					\
		audio__setval((F), (V), AUDIO__SET|AUDIO__RECORD|AUDIO__ERROR)
#define	audio_set_play_eof(F, V)					\
		audio__setval((F), (V), AUDIO__SET|AUDIO__PLAY|AUDIO__EOF)

/* The value can only be set to one.  It is reset to zero on close(). */
#define	audio_set_play_waiting(F, V)					\
		audio__setval((F), (V), AUDIO__SET|AUDIO__PLAY|AUDIO__WAITING)
#define	audio_set_record_waiting(F, V)					\
		audio__setval((F), (V), AUDIO__SET|AUDIO__RECORD|AUDIO__WAITING)


/*
 * Gain routines take double values, mapping the valid range of gains
 * to a floating-point value between zero and one, inclusive.
 * The value returned will likely be slightly different than the value set.
 * This is because the value is quantized by the device.
 *
 * Make sure that 'V' is a (double *)!
 */
#define	audio_get_play_gain(F, V)					\
		audio__setgain((F), (V), AUDIO__PLAY|AUDIO__GAIN)
#define	audio_get_record_gain(F, V)					\
		audio__setgain((F), (V), AUDIO__RECORD|AUDIO__GAIN)
#define	audio_get_monitor_gain(F, V)					\
		audio__setgain((F), (V), AUDIO__MONGAIN)

#define	audio_set_play_gain(F, V)					\
		audio__setgain((F), (V), AUDIO__SET|AUDIO__PLAY|AUDIO__GAIN)
#define	audio_set_record_gain(F, V)					\
		audio__setgain((F), (V), AUDIO__SET|AUDIO__RECORD|AUDIO__GAIN)
#define	audio_set_monitor_gain(F, V)					\
		audio__setgain((F), (V), AUDIO__SET|AUDIO__MONGAIN)

/*
 * The following macros flush the audio play and/or record queues.
 * Note that requests to flush a channel that is not open will have no effect.
 */
#define	audio_flush(F)							\
		audio__flush((F), AUDIO__PLAYREC)
#define	audio_flush_play(F)						\
		audio__flush((F), AUDIO__PLAY)
#define	audio_flush_record(F)						\
		audio__flush((F), AUDIO__RECORD)


/* The following is used for 'which' arguments to get/set info routines */
#define	AUDIO__PLAY		(0x10000)
#define	AUDIO__RECORD		(0x20000)
#define	AUDIO__PLAYREC		(AUDIO__PLAY | AUDIO__RECORD)

#define	AUDIO__PORT		(1)
#define	AUDIO__SAMPLES		(2)
#define	AUDIO__ERROR		(3)
#define	AUDIO__EOF		(4)
#define	AUDIO__OPEN		(5)
#define	AUDIO__ACTIVE		(6)
#define	AUDIO__WAITING		(7)
#define	AUDIO__GAIN		(8)
#define	AUDIO__MONGAIN		(9)
#define	AUDIO__PAUSE		(10)
#define	AUDIO__RESUME		(11)

#define	AUDIO__SET		(0x80000000)
#define	AUDIO__SETVAL_MASK	(0xff)

#endif /*!_multimedia_audio_io_h*/
