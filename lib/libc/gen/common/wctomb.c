/*
 * wctomb
 */

#if !defined(lint) && defined(SCCSIDS)
static  char *sccsid = "@(#)wctomb.c 1.1 92/07/30 SMI";
#endif 

#include <sys/types.h>
#include "codeset.h"
#include "mbextern.h"

int
wctomb(s, pwc)
	char *s;
	wchar_t  pwc;
{
	char *handle;		/* handle */
	int (*p)();
	int ret;

	switch (_code_set_info.code_id) {
	case CODESET_NONE:
		/*
		 * Default code set, 
		 */
		if (s == NULL)
			return (0);	/* No state dependency */
		else {
			*s = (char) (pwc & 0x00ff);
			return (1);
		}
	case CODESET_EUC:
		/*
		 * EUC code set
		 */
		if (s == NULL)
			return (0);	/* No state dependecy */
		return(_wctomb_euc(s, pwc));
		break;

	case CODESET_XCCS:
		/*
		 * XCCS code set
		 */
		if (s == 0)
			return (0);	/* No state dependecy */
		return(_wctomb_xccs(s, pwc));
		break;

	case CODESET_ISO2022:
		/*
		 * ISO family
		 */
		if (s == 0)
			return (1);	/* State dependant */
		return(_wctomb_iso(s, pwc));
		break;

	default:
#ifdef PIC		/* IF DYNAMIC */
		/*
		 * User defined code set
		 */
		 handle = _ml_open_library();
		 if (handle == (char *)NULL)
			return(ERROR_NO_LIB);	/* No user library */
		 p = (int (*)()) dlsym(handle, "_wctomb");
		 if (p == (int (*)()) NULL)
			return(ERROR_NO_SYM);
		 ret = (*p)(s, pwc);
		 return (ret);
		 break;
#else			/* IF STATIC, use default */
		/*
		 * Default code set, 
		 */
		if (s == NULL)
			return (0);	/* No state dependency */
		else {
			*s = (char) (pwc & 0x00ff);
			return (1);
		}
		break;
#endif	PIC
	}
	/* NOTREACHED */
}
