/*
 *       @(#)fpu.c 1.2 91/07/26 Copyright(c) Sun Microsystems, Inc.
 *
 */
#include <stdio.h>
#include <math.h>
#include "mptest.h"
#include "mptest_msg.h"
#include "sdrtns.h"

extern int number_processors;

/*
 * This function performs the single and double precision linpack test.
 */
LINPACK(process_number, cpu_mask)
int process_number;
unsigned cpu_mask;
{

    REAL residn, resid, eps, x11, xn1;
    int cpu;

    cpu = nint(log2((double)cpu_mask));

    send_message(0, VERBOSE, "FPU %s Precision Test: Process %d, Processor %d",
                 PREC, process_number, cpu);

    LINSUB(&residn,&resid,&eps,&x11,&xn1);

    send_message(0, DEBUG, "Process %d, Processor %d, %s %25.16e %25.16e %25.16e %25.16e %25.16e",
                 process_number, cpu, PREC, residn, resid, eps, x11, xn1);

    if ((residn == RESIDN) && (resid == RESID) && (eps == EPS) && (x11 == X11) && (xn1 == XN1))
    {
        send_message(0, VERBOSE, "%s FPU test: Process %d, Processor %d, PASSES",
                     PREC, process_number, cpu);
        return (0);
    }
    else
    {
        send_message(-LINPACK_ERR, ERROR, linpack_err, PREC, process_number, cpu);
        return (-1);
    }
}
 
/*
 * Function to begin linpack calculation.
 */
LINSUB(residn,resid,eps,x11,xn1)
REAL *residn, *resid, *eps, *x11, *xn1;
{
 
    REAL a[200][201],b[200],x[200];
    REAL norma,normx,abs;
    int ipvt[200],n,i,info,lda;
 
    lda = 201;
    n = 100;
 
    MATGEN((REAL *)a,lda,n,b,&norma);
    GEFA((REAL *)a,lda,n,ipvt,&info);
    GESL((REAL *)a,lda,n,ipvt,b);
 
    /* compute a residual to verify results.  */
    for (i = 0; i < n; i++)
    {
        x[i] = b[i];
    }
 
    MATGEN((REAL *)a,lda,n,b,&norma);
    for (i = 0; i < n; i++)
    {
        b[i] = -b[i];
    }
 
    MXPY(n,b,n,lda,x,(REAL *)a);
    *resid = ZERO;
    normx = ZERO;
    for (i = 0; i < n; i++)
    {
        abs = (REAL)fabs((double)b[i]);
        *resid = (*resid > abs) ? *resid : abs;
        abs = (REAL)fabs((double)x[i]);
        normx = (normx > abs) ? normx : abs;
    }
 
    *eps = EPSLON((REAL)ONE);
    *residn = *resid/( n*norma*normx*(*eps) );
    *x11 = x[0] - 1;
    *xn1 = x[n-1] - 1;
 
}
 
/*
 * Function to generate real matrices.
 */
MATGEN(a,lda,n,b,norma)
REAL a[],b[],*norma;
int lda, n;
{
    int init, i, j;
 
    init = 1325;
    *norma = ZERO;
    for (j = 0; j < n; j++)
    {
        for (i = 0; i < n; i++)
        {
            init = 3125*init % 65536;
            a[lda*j+i] = (init - 32768.0)/16384.0;
            *norma = (a[lda*j+i] > *norma) ? a[lda*j+i] : *norma;
        }
    }
       
    for (i = 0; i < n; i++)
    {
        b[i] = ZERO;
    }
 
    for (j = 0; j < n; j++)
    {
        for (i = 0; i < n; i++)
        {
            b[i] = b[i] + a[lda*j+i];
        }
    }
       
}
 
 
/*
 * Function factors a real matrix by gaussian elemination.
 */
GEFA(a,lda,n,ipvt,info)
REAL a[];
int lda,n,ipvt[],*info;
{
    REAL t;
    int IAMAX(),j,k,kp1,l,nm1;
 
    /* gaussian elimination with partial pivoting */
 
    *info = 0;
    nm1 = n - 1;
    if (nm1 >=  0)
    {
        for (k = 0; k < nm1; k++)
        {
            kp1 = k + 1;
 
            /* find l = pivot index */
            l = IAMAX(n-k,&a[lda*k+k]) + k;
            ipvt[k] = l;
 
            /* zero pivot implies this column already triangularized */
            if (a[lda*k+l] != ZERO)
            {
                /* interchange if necessary */
                if (l != k)
                {
                    t = a[lda*k+l];
                    a[lda*k+l] = a[lda*k+k];
                    a[lda*k+k] = t;
                }
 
                /* compute multipliers */
                t = -ONE/a[lda*k+k];
                SCAL(n-(k+1),t,&a[lda*k+k+1]);
 
                /* row elimination with column indexing */
                for (j = kp1; j < n; j++)
                {
                    t = a[lda*j+l];
                    if (l != k)
                    {
                        a[lda*j+l] = a[lda*j+k];
                        a[lda*j+k] = t;
                    }
 
                    AXPY(n-(k+1),t,&a[lda*k+k+1],
                    &a[lda*j+k+1]);
                }
            }
            else
            {
                *info = k;
            }
        }
    }
       
    ipvt[n-1] = n-1;
    if (a[lda*(n-1)+(n-1)] == ZERO) *info = n-1;
 
}

/*
 * Function to solve the real system a * x = b or trans(a) * x = b.
 */
GESL(a,lda,n,ipvt,b)
int lda,n,ipvt[];
REAL a[],b[];
{
    REAL t;
    int k,kb,l,nm1;
 
    nm1 = n - 1;
 
    /* solve  a*x = b first solve l*y = b */
    if (nm1 >= 1)
    {
        for (k = 0; k < nm1; k++)
        {
            l = ipvt[k];
            t = b[l];
            if (l != k)
            {
                b[l] = b[k];
                b[k] = t;
            }
            AXPY(n-(k+1),t,&a[lda*k+k+1],&b[k+1]);
        }
    }
       
    /* now solve  u*x = y */
    for (kb = 0; kb < n; kb++) {
        k = n - (kb + 1);
        b[k] = b[k]/a[lda*k+k];
        t = -b[k];
        AXPY(k,t,&a[lda*k+0],&b[0]);
    }
       
}
 
 
/*
 * Function to perform constant times a vector plus a vector.
 */
AXPY(n,da,dx,dy)
REAL dx[],dy[],da;
int n;
{
    int i, m;
 
    if(n <= 0) return;
    if (da == ZERO) return;
 
    m = n % 4;
    if ( m != 0)
    {
        for (i = 0; i < m; i++)
            dy[i] = dy[i] + da*dx[i];
        if (n < 4) return;
    }
       
    for (i = m; i < n; i = i + 4)
    {
        dy[i] = dy[i] + da*dx[i];
        dy[i+1] = dy[i+1] + da*dx[i+1];
        dy[i+2] = dy[i+2] + da*dx[i+2];
        dy[i+3] = dy[i+3] + da*dx[i+3];
    }
       
}
 
/*
 * Function to scale a vector by a constant.
 */
SCAL(n,da,dx)
REAL da,dx[];
int n;
{
    int i,m;
 
    if (n <= 0) return;
    m = n % 5;
    if (m != 0)
    {
        for (i = 0; i < m; i++)
            dx[i] = da*dx[i];
        if (n < 5) return;
    }
       
    for (i = m; i < n; i = i + 5){
        dx[i] = da*dx[i];
        dx[i+1] = da*dx[i+1];
        dx[i+2] = da*dx[i+2];
        dx[i+3] = da*dx[i+3];
        dx[i+4] = da*dx[i+4];
    }
       
}
 
/*
 * Function to find the index of element having maximum absolute value.
 */
int IAMAX(n,dx)
REAL dx[];
int n;
{
    REAL dmax, abs;
    int i, itemp;
 
    if( n < 1 ) return(-1);
    if(n == 1 ) return(0);
 
    itemp = 0;
    dmax = (REAL)fabs((double)dx[0]);
    for (i = 1; i < n; i++)
    {
        abs = (REAL)fabs((double)dx[i]);
        if (abs > dmax)
        {
            itemp = i;
            dmax = abs;
        }
    }
    return (itemp);
}
 
/*
 * Function to estimate unit roundoff in quantities of size x.
 */
REAL EPSLON(x)
REAL x;
{
 
    REAL a,b,c,eps,abs;
    a = 4.0e0/3.0e0;
    eps = ZERO;
    while (eps == ZERO)
    {
        b = a - ONE;
        c = b + b + b;
        eps = (REAL)fabs((double)(c-ONE));
    }
    abs = (REAL)fabs((double)x);
 
    return(eps*abs);
}
 
 
/*
 * Function to multiply matrix m times vector x and add the result to vector y.
 */
MXPY(n1, y, n2, ldm, x, m)
REAL y[], x[], m[];
int n1, n2, ldm;
{
    int j,i,jmin;
 
    /* cleanup odd vector */
    j = n2 % 2;
    if (j >= 1) {
        j = j - 1;
        for (i = 0; i < n1; i++)
            y[i] = (y[i]) + x[j]*m[ldm*j+i];
    }
 
    /* cleanup odd group of two vectors */
    j = n2 % 4;
    if (j >= 2) {
        j = j - 1;
        for (i = 0; i < n1; i++)
            y[i] = ( (y[i])
            + x[j-1]*m[ldm*(j-1)+i]) + x[j]*m[ldm*j+i];
    }
 
    /* cleanup odd group of four vectors */
    j = n2 % 8;
    if (j >= 4) {
    j = j - 1;
    for (i = 0; i < n1; i++)
        y[i] = ((( (y[i])
        + x[j-3]*m[ldm*(j-3)+i])
        + x[j-2]*m[ldm*(j-2)+i])
        + x[j-1]*m[ldm*(j-1)+i]) + x[j]*m[ldm*j+i];
    }
 
    /* cleanup odd group of eight vectors */
    j = n2 % 16;
    if (j >= 8) {
        j = j - 1;
        for (i = 0; i < n1; i++)
            y[i] = ((((((( (y[i])
            + x[j-7]*m[ldm*(j-7)+i]) + x[j-6]*m[ldm*(j-6)+i])
            + x[j-5]*m[ldm*(j-5)+i]) + x[j-4]*m[ldm*(j-4)+i])
            + x[j-3]*m[ldm*(j-3)+i]) + x[j-2]*m[ldm*(j-2)+i])
            + x[j-1]*m[ldm*(j-1)+i]) + x[j]  *m[ldm*j+i];
    }
        
    /* main loop - groups of sixteen vectors */
    jmin = (n2%16)+16;
    for (j = jmin-1; j < n2; j = j + 16) {
        for (i = 0; i < n1; i++)
            y[i] = ((((((((((((((( (y[i])
            + x[j-15]*m[ldm*(j-15)+i])
            + x[j-14]*m[ldm*(j-14)+i])
            + x[j-13]*m[ldm*(j-13)+i])
            + x[j-12]*m[ldm*(j-12)+i])
            + x[j-11]*m[ldm*(j-11)+i])
            + x[j-10]*m[ldm*(j-10)+i])
            + x[j- 9]*m[ldm*(j- 9)+i])
            + x[j- 8]*m[ldm*(j- 8)+i])
            + x[j- 7]*m[ldm*(j- 7)+i])
            + x[j- 6]*m[ldm*(j- 6)+i])
            + x[j- 5]*m[ldm*(j- 5)+i])
            + x[j- 4]*m[ldm*(j- 4)+i])
            + x[j- 3]*m[ldm*(j- 3)+i])
            + x[j- 2]*m[ldm*(j- 2)+i])
            + x[j- 1]*m[ldm*(j- 1)+i])
            + x[j]   *m[ldm*j+i];
    }
}
