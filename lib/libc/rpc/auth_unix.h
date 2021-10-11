/*	@(#)auth_unix.h 1.1 92/07/30 SMI	*/

/*
 * auth_unix.h, Protocol for UNIX style authentication parameters for RPC
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifndef _rpc_auth_unix_h
#define	_rpc_auth_unix_h

/*
 * The system is very weak.  The client uses no encryption for  it
 * credentials and only sends null verifiers.  The server sends backs
 * null verifiers or optionally a verifier that suggests a new short hand
 * for the credentials.
 */

/* The machine name is part of a credential; it may not exceed 255 bytes */
#define	MAX_MACHINE_NAME 255

/* gids compose part of a credential; there may not be more than 16 of them */
#define	NGRPS 16

/*
 * Unix style credentials.
 */
struct authunix_parms {
	u_long	 aup_time;
	char	*aup_machname;
	u_int	 aup_uid;
	u_int	 aup_gid;
	u_int	 aup_len;
	u_int	*aup_gids;
};

extern bool_t xdr_authunix_parms();

/*
 * If a response verifier has flavor AUTH_SHORT,
 * then the body of the response verifier encapsulates the following structure;
 * again it is serialized in the obvious fashion.
 */
struct short_hand_verf {
	struct opaque_auth new_cred;
};

#endif /*!_rpc_auth_unix_h*/
