#ifndef FRACT
#define FRACT

#ifndef NOFRACT
typedef int fract;

fract fractf(), frmul(), frdiv(), fridivu(), frsdivu(), frsqrt(), getfract();
int floorfr(), ceilingfr(), roundfr(), getfract();
float floatfr();

#define frrsh(i,n) ((i)>>(n))
#define frlsh(i,n) ((i)<<(n))
#define fracti(i) ((fract)((i)<<16))
#define floorfr(x) ((x)>>16)
#define roundfr(x) (((x)+fraction(1,2))>>16)
#define ceilingfr(x) (((x)+fracti(1)-1)>>16)
#define HUGE ((fract)0x7fffffff)
/* these constants given to the accuracy shown (cf. better estimates below) */
#define FRSQRT2 ((fract)92682)	/* exactly 1.414215087890625 */
#define FRPI ((fract)205887)	/* exactly 3.1415863037109375 */
#define FRE ((fract)184696)	/* exactly 2.8182373046875 */

#else NOFRACT

typedef float fract;

#define floorfr(x)	(x<0? ((int)((x)-.999999)): (int)(x))
#define ceilingfr(x)	(x>0? ((int)((x)+.999999)): (int)(x))
#define roundfr(x)	floorfr((x)+.5)
#define fracti(x)	((float)(x))
#define fractf(x)	(x)
#define frmul(x,y)	((x)*(y))
#define frdiv(x,y)	((x)/(y))
#define fridivu(x,y)	((int)((x)/(y)))
#define frsdivu(x,y)	((x)/(y))
#define frsqrt(x)	(sqrt(x))
double sqrt();
#define floatfr(x)	(x)
#define frrsh(i,n) ((i)/(float)(1<<(n)))
#define frlsh(i,n) ((i)*(float)(1<<(n)))
#define getfract(f) 	getdouble(f)
double getdouble();
#define HUGE 9.2233717e18
#define FRSQRT2 1.41421356237309504880
#define FRPI    3.14159265358979323846
#define FRE     2.81828182845904523536
#endif NOFRACT

#define fraction(num, denom) (fracti(num)/(denom))
#define frpercent(pc) fraction(pc, 100)

#endif FRACT
