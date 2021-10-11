/*
 * mbstowcs
 */


#if !defined(lint) && defined(SCCSIDS)
static  char *sccsid = "@(#)mbstowcs.c 1.1 92/07/30 SMI";
#endif 

#include <sys/types.h>
#include "codeset.h"
#include "mbextern.h"

int
mbstowcs(pwcs, s, n)
	wchar_t * pwcs;
	char *s;
	size_t n;
{
	char *handle;		/* handle */
	int (*p)();
	int num = 0;
	int ret;

	switch (_code_set_info.code_id) {
	case CODESET_NONE:
		/*
		 * default code set,
		 */
		 while (*s && num < n) {
			*pwcs++ = (wchar_t)*s++;
			num++;
		 }
		 if (num < n)
			*pwcs = 0;
		return (num);
		break;
	case CODESET_EUC:
		/*
		 * EUC code set
		 */
		 return(_mbstowcs_euc(pwcs, s, n));
		 break;

	case CODESET_XCCS:
		/*
		 * XCCS code set
		 */
		return(_mbstowcs_xccs(pwcs, s, n));
		break;

	case CODESET_ISO2022:
		/*
		 * ISO family
		 */
		return(_mbstowcs_iso(pwcs, s, n));
		break;

	default:
#ifdef PIC
		/*
		 * User defined code set
		 */
		 handle = _ml_open_library();
		 if (handle == (void *)NULL)
			return(ERROR_NO_LIB);	/* No user library */
		 p = (int (*)()) dlsym(handle, "_mbstowcs");
		 if (p == (int (*)()) NULL)
			return(ERROR_NO_SYM);
		 ret = (*p)(pwcs, s, n);
		 return (ret);
		 break;
#else
		/*
		 * default code set,
		 */
		 while (*s && num < n) {
			*pwcs++ = (wchar_t)*s++;
			num++;
		 }
		 if (num < n)
			*pwcs = 0;
		return (num);
		break;
#endif
	}
	/* NOTREACHED */
}
