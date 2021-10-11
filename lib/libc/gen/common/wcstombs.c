/*
 * wcstombs
 */

#if !defined(lint) && defined(SCCSIDS)
static  char *sccsid = "@(#)wcstombs.c 1.1 92/07/30 SMI";
#endif 

#include <sys/types.h>
#include "codeset.h"
#include "mbextern.h"

size_t
wcstombs(s, pwcs, n)
	char *s;
	wchar_t * pwcs;
	size_t n;
{
	char *handle;		/* handle */
	int (*p)();
	int num = 0;
	int ret;

	switch (_code_set_info.code_id) {
	case CODESET_NONE:
		/*
		 * default code set
		 */
		 while (*pwcs && (num < n)) {
			*s++ = *pwcs++ & 0x00ff;
			num++;
		 }
		 if (num < n)
			*s = 0;
		 return (num);
		 break;
	case CODESET_EUC:
		/*
		 * EUC code set
		 */
		 return(_wcstombs_euc(s, pwcs, n));
		 break;

	case CODESET_XCCS:
		/*
		 * XCCS code set
		 */
		return(_wcstombs_xccs(s, pwcs, n));
		break;

	case CODESET_ISO2022:
		/*
		 * ISO family
		 */
		return(_wcstombs_iso(s, pwcs, n));
		break;

	default:
#ifdef	PIC
		/*
		 * User defined code set
		 */
		 handle = _ml_open_library();
		 if (handle == (char *)NULL)
			return(ERROR_NO_LIB);	/* No user library */
		 p = (int (*)()) dlsym(handle, "_wcstombs");
		 if (p == (int (*)()) NULL)
			return(ERROR_NO_SYM);
		 ret = (*p)(s, pwcs, n);
		 return (ret);
		 break;
#else
		/*
		 * default code set
		 */
		 while (*pwcs && (num < n)) {
			*s++ = *pwcs++ & 0x00ff;
			num++;
		 }
		 if (num < n)
			*s = 0;
		 return (num);
		 break;
#endif
	}
	/* NOTREACHED */
}
