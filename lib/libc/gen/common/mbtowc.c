/*
 * mbtowc
 */


#if !defined(lint) && defined(SCCSIDS)
static  char *sccsid = "@(#)mbtowc.c 1.1 92/07/30 SMI";
#endif 

#include <stdlib.h>
#include "codeset.h"
#include "mbextern.h"

#undef	mblen

int
mbtowc(pwc, s, n)
	wchar_t * pwc;
	char *s;
	size_t n;
{
	char *handle;		/* handle */
	int (*p)();
	int ret;

	switch (_code_set_info.code_id) {
	case CODESET_NONE:
#ifdef DEBUG
	printf ("DEFAULT: mbtowc invoked\n");
#endif
		/*
		 * This is a default code set
		 */
		 if (s == NULL)
			return (1);
		 else {
			if (pwc != NULL)
				*pwc = (unsigned char)*s;
			return (1);
		 }
		 break;
	case CODESET_EUC:
#ifdef DEBUG
	printf ("EUC: mbtowc invoked\n");
#endif
		/*
		 * EUC code set
		 */
		 return(_mbtowc_euc(pwc, s, n));
		 break;

	case CODESET_XCCS:
#ifdef DEBUG
	printf ("XCCS: mbtowc invoked\n");
#endif
		/*
		 * XCCS code set
		 */
		return(_mbtowc_xccs(pwc, s, n));
		break;

	case CODESET_ISO2022:
#ifdef DEBUG
	printf ("ISO2022: mbtowc invoked\n");
#endif
		/*
		 * ISO family
		 */
		return(_mbtowc_iso(pwc, s, n));
		break;

	default:
#ifdef PIC	/* if DYNAMIC */
		/*
		 * User defined code set
		 */
		 handle = _ml_open_library();
		 if (handle == (char *)NULL)
			return(ERROR_NO_LIB);	/* No user library */
		 p = (int (*)()) dlsym(handle, "_mbtowc");
		 if (p == (int (*)()) NULL)
			return(ERROR_NO_SYM);
		 ret = (*p)(pwc, s, n);
		 return (ret);
		 break;
#else		/* if STATIC */
		/*
		 * This is a default code set
		 */
		 if (s == NULL)
			return (1);
		 else {
			if (pwc != NULL)
				*pwc = (unsigned char)*s;
			return (1);
		 }
		 break;
#endif PIC	
	}
	/* NOTREACHED */
}

int mblen(s, n)
register char *s; int n;
{
	int val;

	if (_code_set_info.code_id != CODESET_ISO2022)
		return (mbtowc((wchar_t *)0, s, n));
	else {
		/*
		 * ISO's mbtowc() changes 'states'.
		 */
		_savestates();
		val = mbtowc((wchar_t *)0, s, n);
		_restorestates();
		return (val);
	}
}
