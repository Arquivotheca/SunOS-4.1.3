/*
 * Definitions of parameter names for system methods
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

/*      @(#)sys_param_names.h 1.1 92/07/30 SMI */

#ifndef _sys_param_names_h
#define	_sys_param_names_h

#define	DOMAIN_PARAM	"DOMAIN"
#define	HOSTNAME_PARAM  "HOSTNAME"
#define	IPADDR_PARAM	"IPADDR"
#define	TIMESERVER_PARAM	"TIMESERVER"
#define TIMEZONE_PARAM	"TIMEZONE"

#define	SYS_ADD_METHOD	"sys_add"
#define	SYS_HOSTNAME_METHOD	"sys_hostname"
#define	SYS_DOMAIN_METHOD	"sys_domain"
#define	SYS_IPADDR_METHOD	"sys_ipaddr"
#define	SYS_TIMEZONE_METHOD	"sys_tz"
#define	SYS_TIME_METHOD	"sys_time"
#define	SYS_ROOTPASSWD_METHOD	"sys_rootpasswd"

#define	SYSTEM_CLASS_PARAM	"system"

#define	MAXDOMAINLEN	64

#endif /* !_sys_param_names_h */
