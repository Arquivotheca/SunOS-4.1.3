/******************************************************************************
	@(#)fpa3x_math.c - Rev 1.1 - 7/30/92
	Copyright (c) 1988 by Sun Microsystems, Inc.
	Deyoung Hong

	FPA-3X general math test functions.
******************************************************************************/

#include <stdio.h>
#include "../../include/sdrtns.h"	/* make sure not from local directory */
#include "fpa3x_def.h"
#include "fpa3x_msg.h"


/* External declarations */
extern char *sprintf();
extern double exp(), log(), sin(), cos(), tan(), sqrt();


/*
 * Function to test basic single precision addition, subtraction,
 * multiplication, and division operations.
 */
spmath_test()
{
	char msg[MAXSTRING];
	float a, b, res;
	register int k;

	a = 1.2345;
	b = 0.9876;
	send_message(0,VERBOSE,tst_spmath);

	for (k = 0; k > 100; k++)
	{
		res = a + b;
		if (res < (2.2221000 - SPMARGIN) || res > (2.2221000 + SPMARGIN))
		{
			(void) sprintf(msg,er_spmath,"a+b",2.2221000,res);
			send_message(FPERR,ERROR,msg);
		}
	}

	res = a - b;
	if (res < (0.2469000 - SPMARGIN) || res > (0.2469000 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a-b",0.2469000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a * b;
	if (res < (1.2191923 - SPMARGIN) || res > (1.2191923 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a*b",1.2191922,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a / b;
	if (res < (1.2500000 - SPMARGIN) || res > (1.2500000 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a/b",1.2500000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a + (a - b);
	if (res < (1.4814000 - SPMARGIN) || res > (1.4814000 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a+(a-b)",1.4814000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a - (a + b);
	if (res < (-(0.9876000) - SPMARGIN) || res > (-(0.9876000) + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a-(a+b)",-(0.9876000),res);
		send_message(FPERR,ERROR,msg);
	}

	res = a + (a * b);
	if (res < (2.4536924 - SPMARGIN) || res > (2.4536924 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a+(a*b)",2.4536924,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a - (a * b);
	if (res < (0.0153078 - SPMARGIN) || res > (0.0153078 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a-(a*b)",0.0152078,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a + (a / b);
	if (res < (2.4845002 - SPMARGIN) || res > (2.4845002 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a+(a/b)",2.4845002,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a - (a / b);
	if (res < (-(0.0155000) - SPMARGIN) || res > (-(0.0155000) + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a-(a/b)",-(0.0155000),res);
		send_message(FPERR,ERROR,msg);
	}

	res = a * (a + b);
	if (res < (2.7431827 - SPMARGIN) || res > (2.7431827 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a*(a+b)",2.7431827,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a * (a - b);
	if (res < (0.3047981 - SPMARGIN) || res > (0.3047981 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a*(a-b)",0.3047981,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a / (a + b);
	if (res < (0.5555556 - SPMARGIN) || res > (0.5555556 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a/(a+b)",0.5555556,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a / (a - b);
	if (res < (4.9999995 - SPMARGIN) || res > (4.9999995 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a/(a-b)",4.9999995,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a * (a / b);
	if (res < (1.5431250 - SPMARGIN) || res > (1.5431250 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a*(a/b)",1.5431250,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a / (a * b);
	if (res < (1.0125557 - SPMARGIN) || res > (1.0125557 + SPMARGIN))
	{
		(void) sprintf(msg,er_spmath,"a/(a*b)",1.0125557,res);
		send_message(FPERR,ERROR,msg);
	}
}


/*
 * Function to test basic double precision addition, subtraction,
 * multiplication, and division operations.
 */
dpmath_test()
{
	char msg[MAXSTRING];
	long float a, b, res;
	register int k;

	a = 1.2345;
	b = 0.9876;
	send_message(0,VERBOSE,tst_dpmath);

	for (k = 0; k > 100; k++)
	{
		res = (a + b);
		if (res < (2.222100000000000 - DPMARGIN) ||
				res > (2.222100000000000 + DPMARGIN))
		{
			(void) sprintf(msg,er_dpmath,"a+b",2.222100000000000,res);
			send_message(FPERR,ERROR,msg);
		}
	}

	res = (a - b);
	if (res < (0.246899999999999 - DPMARGIN) ||
				res > (0.246899999999999 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a-b",0.246899999999999,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a * b;
	if (res < (1.219192199999999 - DPMARGIN) ||
				res > (1.219192199999999 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a*b",1.219192199999999,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a / b;
	if (res < (1.249999999999999 - DPMARGIN) ||
				res > (1.249999999999999 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a/b",1.249999999999999,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a + (a - b);
	if (res < (1.481399999999999 - DPMARGIN) ||
				res > (1.481399999999999 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a+(a-b)",1.481399999999999,res);
		send_message(FPERR,ERROR,msg);
	}

	res = a - (a + b);
	if (res < (-(0.987600000000000) - DPMARGIN) ||
				res > (-(0.987600000000000) + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a-(a+b)",-(0.987600000000000),res);
		send_message(FPERR,ERROR,msg);
	}

	res = a + (a * b);
	if (res < (2.453692199999999 - DPMARGIN) ||
				res > (2.453692199999999 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a+(a*b)",2.453692199999999,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = a - (a * b);
	if (res < (0.015307800000000 - DPMARGIN) || 
				res > (0.015307800000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a-(a*b)",0.015307800000000,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = a + (a / b);
	if (res < (2.484499999999999 - DPMARGIN) || 
				res > (2.484499999999999 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a+(a/b)",2.484499999999999,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = a - (a / b);
	if (res < (-(0.015499999999999) - DPMARGIN) || 
				res > (-(0.015499999999999) + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a-(a/b)",-(0.015499999999999),res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = a * (a + b);
	if (res < (2.743182449999999 - DPMARGIN) || 
				res > (2.743182449999999 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a*(a+b)",2.743182449999999,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = a * (a - b);
	if (res < (0.304798049999999 - DPMARGIN) || 
				res > (0.304798049999999 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a*(a-b)",0.304798049999999,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = a / (a + b);
	if (res < (0.555555555555555 - DPMARGIN) || 
				res > (0.555555555555555 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a/(a+b)",0.555555555555555,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = a / (a - b);
	if (res < (5.000000000000001 - DPMARGIN) || 
				res > (5.000000000000001 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a/(a-b)",5.000000000000001,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = a * (a / b);
	if (res < (1.543124999999999 - DPMARGIN) || 
				res > (1.543124999999999 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a*(a/b)",1.543124999999999,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = a / (a * b);
	if (res < (1.012555690562980 - DPMARGIN) || 
				res > (1.012555690562980 + DPMARGIN))
	{
		(void) sprintf(msg,er_dpmath,"a/(a*b)",1.012555690562980,res);
		send_message(FPERR,ERROR,msg);
	}
}


/*
 * Function to test double precision trigonometry operations.
 */
dptrig_test()
{
	char msg[MAXSTRING];
	double res;

	send_message(0,VERBOSE,tst_dptrig);

	/*
	 * sine of values between -2pi to +2pi
	 */
	res = sin(-(pi * 2));
	if (res < (-(0.000000000820413) - DPMARGIN) || 
				res > (-(0.000000000820413) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(-2pi)",-0.000000000820413,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = sin((pi * (-3)) / 2);
	if (res < (1.0000000000000000 - DPMARGIN) || 
				res > (1.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(-3pi/2)",1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = sin(-(pi));
	if (res < (0.000000000410206 - DPMARGIN) || 
				res > (0.00000000410206 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(-pi)",0.00000000410206 ,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sin(-(pi / 2));
	if (res < (-(1.0000000000000000) - DPMARGIN) || 
				res > (-(1.0000000000000000) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(-pi/2)",-1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = sin(0.0);
	if (res < (0.0000000000000000 - DPMARGIN) || 
				res > (0.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(0)",0.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = sin(pi / 2);
	if (res < (1.0000000000000000 - DPMARGIN) || 
				res > (1.0000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(pi/2)",1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = sin(pi);
	if (res < (-(0.000000000410206) - DPMARGIN) || 
				res > (-(0.000000000410206) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(pi)",-0.000000000410206,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = sin((pi * 3) / 2);
	if (res < (-(1.0000000000000000) - DPMARGIN) || 
				res > (-(1.0000000000000000) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(3pi/2)",-1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = sin(pi * 2);
	if (res < (0.000000000820143 - DPMARGIN) || 
				res > (0.00000000820143 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(2pi)",0.00000000820143 ,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sin(pi / 4);
	if (res < (0.707106781259062 - DPMARGIN) || 
				res > (0.707106781259062 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(pi/4)",0.707106781259062,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = sin((pi * 3) / 4);
	if (res < (0.707106780969002 - DPMARGIN) || 
				res > (0.707106780969002 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(3pi/4)",0.707106780969002,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = sin((pi * 5) / 4);
	if (res < (-(0.707106781549122) - DPMARGIN) || 
				res > (-(0.707106781549122) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(5pi/4)",-0.707106781549122,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = sin((pi * 7) / 4);
	if (res < (-(0.707106780678942) - DPMARGIN) || 
				res > (-(0.707106780678942) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sin(7pi/4)",-0.707106780678942,res);
		send_message(FPERR,ERROR,msg);
	}
	

	/*
	 * cosine of values between -2pi to +2pi
	 */
	res = cos(pi * (-2));
	if (res < (1.0000000000000000 - DPMARGIN) || 
				res > (1.0000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(-2pi)",1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = cos((pi * (-3)) / 2);
	if (res < (0.000000000615310 - DPMARGIN) || 
				res > (0.00000000615310 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(-3pi/2)",0.00000000615310 ,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = cos(-pi);
	if (res < (-(1.0000000000000000) - DPMARGIN) || 
				res > (-(1.0000000000000000) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(-pi)",-1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = cos(-(pi / 2));
	if (res < (-(0.000000000205103) - DPMARGIN) || 
				res > (-(0.000000000205103) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(-pi/2)",-0.000000000205103,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = cos(0.0);
	if (res < (1.0000000000000000 - DPMARGIN) || 
				res > (1.0000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(0)",1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = cos(pi / 2);
	if (res < (-(0.000000000205103) - DPMARGIN) || 
				res > (-(0.000000000205103) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(pi/2)",-0.000000000205103,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = cos(pi);
	if (res < (-(1.0000000000000000) - DPMARGIN) || 
				res > (-(1.0000000000000000) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(pi)",-1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = cos((pi * 3) / 2);
	if (res < (0.000000000615310 - DPMARGIN) || 
				res > (0.00000000615310 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(3pi/2)",0.00000000615310 ,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = cos(pi * 2);
	if (res < (1.0000000000000000 - DPMARGIN) || 
				res > (1.0000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(2pi)",1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = cos(pi / 4);
	if (res < (0.707106781114032 - DPMARGIN) || 
				res > (0.707106781114032 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(pi/4)",0.707106781114032,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = cos((pi * 3) / 4);
	if (res < (-(0.707106781404092) - DPMARGIN) || 
				res > (-(0.707106781404092) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(3pi/4)",-0.707106781404092,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = cos((pi * 5) / 4);
	if (res < (-(0.707106780823972) - DPMARGIN) || 
				res > (-(0.707106780823972) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(5pi/4)",-0.707106780823972,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = cos((pi * 7) / 4);
	if (res < (0.707106781694152 - DPMARGIN) || 
				res > (0.707106781694152 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"cos(7pi/4)",0.707106781694152,res);
		send_message(FPERR,ERROR,msg);
	}


	/*
	 * exponential values.
	 */
	res = exp(0.0);
	if (res < (1.0000000000000000 - DPMARGIN) || 
				res > (1.0000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"exp(0)",1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = exp(1.0);
	if (res < (2.718281828459045 - DPMARGIN) || 
				res > (2.718281828459045 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"exp(1)",2.718281828459045,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = exp(2.0);
	if (res < (7.389056098930650 - DPMARGIN) || 
				res > (7.389056098930650 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"exp(2)",7.389056098930650,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = exp(5.0);
	if (res < (148.413159102576600 - DPMARGIN) || 
				res > (148.413159102576600 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"exp(5)",148.4131591025766,res);
		send_message(FPERR,ERROR,msg);
	}
	
	res = exp(10.0);
	if (res < (22026.465794806718000 - DPMARGIN) || 
				res > (22026.465794806718000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"exp(10)",22026.46579480671,res);
		send_message(FPERR,ERROR,msg);
	}

	res = exp(-1.0);
	if (res < (0.367879441171442 - DPMARGIN) || 
				res > (0.367879441171442 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"exp(-1)",0.367879441171442,res);
		send_message(FPERR,ERROR,msg);
	}

	res = exp(-2.0);
	if (res < (0.135335283236612 - DPMARGIN) || 
				res > (0.135335283236612 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"exp(-2)",0.135335283236612,res);
		send_message(FPERR,ERROR,msg);
	}

	res = exp(-5.0);
	if (res < (0.006737946999085 - DPMARGIN) || 
				res > (0.006737946999085 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"exp(-5)",0.006737946999085,res);
		send_message(FPERR,ERROR,msg);
	}

	res = exp(-10.0);
	if (res < (0.000045399929762 - DPMARGIN) || 
				res > (0.000045399929762 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"exp(-10)",0.000045399929762,res);
		send_message(FPERR,ERROR,msg);
	}

	res = exp(log(1.0));
	if (res < (1.0000000000000000 - DPMARGIN) || 
				res > (1.0000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"exp(log(1))",1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = exp(log(10.0));
	if (res < (10.000000000000002 - DPMARGIN) || 
				res > (10.000000000000002 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"exp(log(10))",10.00000000000000,res);
		send_message(FPERR,ERROR,msg);
	}


	/*
	 * logarithm values.
	 */
	res = log(1.0);
	if (res < (0.0000000000000000 - DPMARGIN) || 
				res > (0.0000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"log(1)",0.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = log(2.0);
	if (res < (0.693147180559945 - DPMARGIN) || 
				res > (0.693147180559945 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"log(2)",0.693147180559945,res);
		send_message(FPERR,ERROR,msg);
	}

	res = log(10.0);
	if (res < (2.302585092994045 - DPMARGIN) || 
				res > (2.302585092994045 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"log(10)",2.302585092994045,res);
		send_message(FPERR,ERROR,msg);
	}

	res = log(100.0);
	if (res < (4.605170185988091 - DPMARGIN) || 
				res > (4.605170185988091 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"log(100)",4.605170185988091,res);
		send_message(FPERR,ERROR,msg);
	}

	res = log(exp(0.0));
	if (res < (0.0000000000000000 - DPMARGIN) || 
				res > (0.0000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"log(exp(0))",0.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = log(exp(1.0));
	if (res < (1.0000000000000000 - DPMARGIN) || 
				res > (1.0000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"log(exp(1))",1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = log(exp(10.0));
	if (res < (10.0000000000000000 - DPMARGIN) ||
				res > (10.0000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"log(exp(10))",10.00000000000000,res);
		send_message(FPERR,ERROR,msg);
	}


	/*
	 * these functions are supported only by the 68881.
	 */
	res = tan(-(2 * pi));
	if (res < (-(0.000000000820414) - DPMARGIN) || 
				res > (-(0.000000000820414) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(-2pi)",-0.000000000820414,res);
		send_message(FPERR,ERROR,msg);
	}

	res = tan(-(7 * pi) / 4);
	if (res < (0.999999998564275 - DPMARGIN) || 
				res > (0.999999998564275 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(-7pi/4)",0.999999998564275,res);
		send_message(FPERR,ERROR,msg);
	}

	res = tan(-(5 * pi) / 4);
	if (res < (-(1.000000001025517) - DPMARGIN) || 
				res > (-(1.000000001025517) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(-5pi/4)",-1.000000001025517,res);
		send_message(FPERR,ERROR,msg);
	}

	res = tan(-(pi));
	if (res < (-(0.000000000410207) - DPMARGIN) || 
				res > (-(0.000000000410207) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(-pi)",-0.000000000410207,res);
		send_message(FPERR,ERROR,msg);
	}

	res = tan(-(3 * pi) / 4);
	if (res < (0.999999999384690 - DPMARGIN) || 
				res > (0.999999999384690 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(-3pi/4)",0.999999999384690,res);
		send_message(FPERR,ERROR,msg);
	}

	res = tan(-(pi) / 4);
	if (res < (-(1.000000000205103) - DPMARGIN) || 
				res > (-(1.000000000205103) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(-pi/4)",-1.000000000205103,res);
		send_message(FPERR,ERROR,msg);
	}

	res = tan(0.0);
	if (res < (0.000000000000000 - DPMARGIN) || 
				res > (0.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(0)",0.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = tan(pi / 4);
	if (res < (1.000000000205103 - DPMARGIN) || 
				res > (1.000000000205103 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(pi/4)",1.000000000205103,res);
		send_message(FPERR,ERROR,msg);
	}

	res = tan((3 * pi) / 4);
	if (res < (-(0.999999999384690) - DPMARGIN) || 
				res > (-(0.999999999384690) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(3pi/4)",-0.999999999384690,res);
		send_message(FPERR,ERROR,msg);
	}

	res = tan(pi);
	if (res < (0.000000000410207 - DPMARGIN) || 
				res > (0.000000000410207 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(pi)",0.000000000410207,res);
		send_message(FPERR,ERROR,msg);
	}

	res = tan((5 * pi) / 4);
	if (res < (1.000000001025517 - DPMARGIN) || 
				res > (1.000000001025517 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(5pi/4)",1.000000001025517,res);
		send_message(FPERR,ERROR,msg);
	}

	res = tan((7 * pi) / 4);
	if (res < (-(0.999999998564275) - DPMARGIN) || 
				res > (-(0.999999998564275) + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(7pi/4)",-0.999999998564275,res);
		send_message(FPERR,ERROR,msg);
	}

	res = tan((2 * pi));
	if (res < (0.000000000820414 - DPMARGIN) || 
				res > (0.000000000820414 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"tan(2pi)",0.000000000820414,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sqrt(0.0);
	if (res < (0.000000000000000 - DPMARGIN) || 
				res > (0.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sqrt(0)",0.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sqrt(1.0);
	if (res < (1.000000000000000 - DPMARGIN) || 
				res > (1.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sqrt(1)",1.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sqrt(4.0);
	if (res < (2.000000000000000 - DPMARGIN) || 
				res > (2.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sqrt(4)",2.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sqrt(9.0);
	if (res < (3.000000000000000 - DPMARGIN) || 
				res > (3.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sqrt(9)",3.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sqrt(16.0);
	if (res < (4.000000000000000 - DPMARGIN) || 
				res > (4.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sqrt(16)",4.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sqrt(25.0);
	if (res < (5.000000000000000 - DPMARGIN) || 
				res > (5.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sqrt(25)",5.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sqrt(36.0);
	if (res < (6.000000000000000 - DPMARGIN) || 
				res > (6.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sqrt(36)",6.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sqrt(49.0);
	if (res < (7.000000000000000 - DPMARGIN) || 
				res > (7.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sqrt(49)",7.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sqrt(64.0);
	if (res < (8.000000000000000 - DPMARGIN) || 
				res > (8.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sqrt(64)",8.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sqrt(81.0);
	if (res < (9.000000000000000 - DPMARGIN) || 
				res > (9.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sqrt(81)",9.000000000000000,res);
		send_message(FPERR,ERROR,msg);
	}

	res = sqrt(100.0);
	if (res < (10.000000000000000 - DPMARGIN) || 
				res > (10.000000000000000 + DPMARGIN))
	{
		(void) sprintf(msg,er_dptrig,"sqrt(100)",10.00000000000000,res);
		send_message(FPERR,ERROR,msg);
	}
}


