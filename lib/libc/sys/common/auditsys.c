#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)auditsys.c 1.1 92/07/30 SMI"; 
#endif

#include	<sys/syscall.h>
#include	<sys/types.h>
#include	<sys/label.h>
#include	<sys/audit.h>

/*
 * Opcodes for auditsys system call
 */
#define AUS_AUDITSVC		1
#define AUS_AUDIT		2
#define AUS_GETAUID		3
#define AUS_SETAUID		4
#define AUS_SETUSERAUDIT	5
#define AUS_SETAUDIT		6
#define AUS_AUDITON		7

auditsvc(fd, limit)
	int		fd;
	int		limit;
{
	return(syscall(SYS_auditsys, AUS_AUDITSVC, fd, limit));
}

audit(record)
	audit_record_t	*record;
{
	return(syscall(SYS_auditsys, AUS_AUDIT, record));
}

getauid()
{
	return(syscall(SYS_auditsys, AUS_GETAUID));
}

setauid(auid)
	int		auid;
{
	audit_state_t	state;

	if (auid == AU_NOAUDITID) {
		state.as_success = 0;
		state.as_failure = 0;
		setaudit(&state);
	}
	return(syscall(SYS_auditsys, AUS_SETAUID, auid));
}

setuseraudit(uid, state)
	int		uid;
	audit_state_t	*state;
{
	return(syscall(SYS_auditsys, AUS_SETUSERAUDIT, uid, state));
}

setaudit(state)
	audit_state_t	*state;
{
	return(syscall(SYS_auditsys, AUS_SETAUDIT, state));
}

auditon(condition)
	int		condition;
{
	return(syscall(SYS_auditsys, AUS_AUDITON, condition));
}

