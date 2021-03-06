/*
Copyright (c) 2017 ETH Zurich
Sascha Brueck, Mauro Calderara, Mohammad Hossein Bani-Hashemian, and Mathieu Luisier

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef __BLAS
#define __BLAS

#include "mpi.h"
#include <iostream>
#include "Types.H"
#ifdef HAVE_SPLITSOLVE
#include "CWC_utility.H"
#include "magma.h"
#endif

namespace MYBLAS {

extern "C" {
//Blas
    void fortran_name(dcopy,DCOPY)(int *n,double *dx,int *incx,double *dy,int *incy);
    void fortran_name(daxpy,DAXPY)(int *n,double *da,double *dx,int *incx,double *dy,\
                                      int *incy);
    double fortran_name(dnrm2,DNRM2)(int *n,double *dx,int *incx);
    double fortran_name(dznrm2,DZNRM2)(int *n,CPX *dx,int *incx);
    void fortran_name(dscal,DSCAL)(int *n,double *da,double *dx,int *incx);
    void fortran_name(zscal,ZSCAL)(int *n,CPX *da,CPX *dx,int *incx);
    void fortran_name(dgemm,DGEMM)(char *transa, char *transb, int *m, int *n, int *k, \
                                   double *alpha, double *a, int *lda, double *b, int *ldb, \
                                   double *beta, double *c, int *ldc);
    void fortran_name(dsymm,DSYMM)(char *side, char *uplo, int *m, int *n, double *alpha, \
                                   double *a, int *lda, double *b, int *ldb, double *beta, \
                                   double *c, int *ldc);
    void fortran_name(zgemm,ZGEMM)(char *transa, char *transb, int *m, int *n, int *k, \
                                   CPX *alpha, CPX *a, int *lda, CPX *b, int *ldb, \
                                   CPX *beta, CPX *c, int *ldc);
    void fortran_name(zhemm,ZHEMM)(char *side, char *uplo, int *m, int *n, CPX *alpha, \
                                   CPX *a, int *lda, CPX *b, int *ldb, CPX *beta, \
                                   CPX *c, int *ldc);
    void fortran_name(dgemv,DGEMV)(char *trans, int *m, int *n, double *alpha, double *a, \
                                   int *lda, double *x, int *incx, double *beta, double *y, \
                                   int *incy);
    
    void fortran_name(zgemv,ZGEMV)(char *trans, int *m, int *n, CPX *alpha, CPX *a, \
                                   int *lda, CPX *x, int *incx, CPX *beta, CPX *y, \
                                   int *incy);

    double fortran_name(ddot,DDOT)(int *n,double *x,int *incx,double *y,int *incy);
    CPX fortran_name(zdotc,ZDOTC)(int *n,CPX *x,int *incx,CPX *y,int *incy);
    void fortran_name(zcopy,ZCOPY)(int *n,CPX *dx,int *incx,CPX *dy,int *incy);
    void fortran_name(zaxpy,ZAXPY)(int *n, CPX *alpha, CPX *x, int *incx, CPX *y, int *incy);
    double fortran_name(dasum,DASUM)(int *n,double *dx,int *incx);
// SAB
    double fortran_name(dzasum,DZASUM)(int *n,CPX *dx,int *incx);

//Lapack
    void fortran_name(dgetrf,DGETRF)(int *m, int *n, double *a, int *lda, int *ipiv, int *info);
    void fortran_name(dgetrs,DGETRS)(char *trans, int *n, int *nrhs, double *a, int *lda, \
                                     int *ipiv, double *b, int *ldb, int *info);
    void fortran_name(zgetrf,ZGETRF)(int *m, int *n, CPX *a, int *lda, int *ipiv, int *info);
    void fortran_name(zgetrs,ZGETRS)(char *trans, int *n, int *nrhs, CPX *a, int *lda, \
                                     int *ipiv, CPX *b, int *ldb, int *info);
    void fortran_name(zgetri,ZGETRI)(int *n,CPX *a,int *lda,int *ipiv,CPX *work,int *lwork,\
                                     int *info);
    void fortran_name(dgeev,DGEEV)(char *jobvl, char *jobvr, int *n, double *a, int *lda, \
                                   double *wr, double *wi, double *vl, int *ldvl, double *vr, \
                                   int *ldvr, double *work, int *lwork, int *info);
    void fortran_name(dsyev,DSYEV)(char *JOBZ,char *UPLO,int *N,double *A,int *LDA,double *W,\
				   double *WORK,int *LWORK,int *INFO);
    void fortran_name(dggev,DGGEV)(char* jobvl, char *jobvr, int *n, double *a, int *lda, \
                                   double *b, int *ldb, double *alphar, double *alphai, \
                                   double *beta, double *vl, int *ldvl, double *vr, int *ldvr, \
                                   double *work, int *lwork, int *info);
    void fortran_name(zggev,ZGGEV)(char* jobvl, char *jobvr, int *n, CPX *a, int *lda, \
                                   CPX *b, int *ldb, CPX *alpha, \
                                   CPX *beta, CPX *vl, int *ldvl, CPX *vr, int *ldvr, \
                                   CPX *work, int *lwork, double *rwork, int *info);
    void fortran_name(zgeev,ZGEEV)(char *jobvl, char *jobvr, int *n, CPX *a, int *lda, \
                                   CPX *w, CPX *vl, int *ldvl, CPX *vr, int *ldvr, CPX *work, \
                                   int *lwork, double *rwork, int *info);
    void fortran_name(zheev,ZHEEV)(char *jobvl, char *uplo, int *n, CPX *a, int *lda, \
                                   double *w, CPX *work, int *lwork, double *rwork, int *info);
    void fortran_name(dgetri,DGETRI)(int *n,double *a,int *lda,int *ipiv,double *work,int *lwork,\
                                     int *info);
    void fortran_name(dsytri,DSYTRI)(char *uplo,int *n,double *a,int *lda,int *ipiv,double *work,\
                                     int *info);
    void fortran_name(zhetrf,ZHETRF)(char *uplo,int *n,CPX *a,int *lda,int *ipiv,CPX *work,\
                                     int *lwork,int *info);
    void fortran_name(zhetri,ZHETRI)(char *uplo,int *n,CPX *a,int *lda,int *ipiv,CPX *work,\
                                     int *info);
    void fortran_name(zhetrs,ZHETRS)(char *uplo,int *n,int *nrhs,CPX *a,int *lda,int *ipiv,\
                                     CPX *b,int *ldb,int *info);
    void fortran_name(dsysv,DSYSV)(char *uplo,int *n,int *nrhs,double *a,int *lda,int *ipiv,\
                                   double *b,int *ldb,double *work,int *lwork,int *info);
    void fortran_name(dsytrf,DSYTRF)(char *uplo,int *n,double *a,int *lda,int *ipiv,double *work,\
                                     int *lwork,int *info);
    void fortran_name(dsytrs,DSYTRS)(char *uplo,int *n,int *nrhs,double *a,int *lda,int *ipiv,\
                                     double *b,int *ldb,int *info);
    void fortran_name(dstebz,DSTEBZ)(char *range,char *order,int *iter,double *vl,double *vu,int *il,int *iu,\
                                     double *abstol,double *diag,double *offd,int *neval,int *nsplit,\
                                     double *eval,int *iblock,int *isplit,double *work,int *iwork,int *info);
    void fortran_name(zlarnv,ZLARNV)(int*,int*,int*,CPX*);
// SAB
    void fortran_name(dsygv,DSYGV)(int *itype,char *jobz,char *uplo,int *n,double *a,int *lda,double *b,int *ldb,\
                                   double *w,double *work,int *lwork,int *info);
    void fortran_name(zhegv,ZHEGV)(int *itype,char *jobz,char *uplo,int *n,CPX *a,int *lda,CPX *b,int *ldb,\
                                   double *w,CPX *work,int *lwork,double *rwork,int *info);
    void fortran_name(zhegvx,ZHEGVX)(int *itype,char *jobz,char *range,char *uplo,int *n,CPX *a,int *lda,\
                                     CPX *b,int *ldb, double *vl, double *vu, int *il,int *iu,double *abstol,\
                                     int *m,double *w,CPX *z,int *ldz,CPX *work,int *lwork,double *rwork,\
                                     int *iwork,int *ifail,int *info);
    void fortran_name(dgesdd,DGESDD)(char *jobz,int *m,int *n,double *a,int *lda,double *s,double *u,int *ldu,\
                                     double *vt,int *ldvt,double *work,int *lwork,int *iwork,int *info);
    void fortran_name(zgesdd,ZGESDD)(char *jobz,int *m,int *n,CPX *a,int *lda,double *s,CPX *u,int *ldu,\
                                     CPX *vt,int *ldvt,CPX *work,int *lwork,double *rwork,int *iwork,int *info);
    double fortran_name(dlamch,DLAMCH)(char*);
    void fortran_name(dlasrt,DLASRT)(char*,int*,double*,int*);

// MMC
    void fortran_name(dsterf,DSTERF)(int* N, double* D, double* E, int* INFO);
    void fortran_name(dsteqr,DSTEQR)(char* COMPZ, int* N, double* D, 
                                     double* E, double* Z, int* LDZ,
                                     double* WORK, int* INFO);
    void fortran_name(dlacpy,DLACPY)(char* UPLO, int* M, int* N, double* A,
                                     int* LDA, double* B, int* LDB);
    void fortran_name(zlacpy,ZLACPY)(char* UPLO, int* M, int* N, CPX* A,
                                     int* LDA, CPX* B, int* LDB);

}

/*My Blas*******************************************************************************************/

inline void c_icopy(int n,int *dx,int incx,int *dy,int incy)
{
    int i;
    
    for(i=0;i<n;i++) dy[i*incy] = dx[i*incx];
}

/*Blas*******************************************************************************************/
    
inline void c_dcopy(int n,double *dx,int incx,double *dy,int incy)
{
    fortran_name(dcopy,DCOPY)(&n,dx,&incx,dy,&incy);
}

/************************************************************************************************/

inline void c_daxpy(int n,double da,double *dx,int incx,double *dy,int incy)
{
    fortran_name(daxpy,DAXPY)(&n,&da,dx,&incx,dy,&incy);
}

/************************************************************************************************/

inline double c_dnrm2(int n,double* dx,int incx)
{
    return fortran_name(dnrm2,DNRM2)(&n,dx,&incx);
}

/************************************************************************************************/

inline double c_dznrm2(int n,CPX* dx,int incx)
{
    return fortran_name(dznrm2,DZNRM2)(&n,dx,&incx);
}

/************************************************************************************************/

inline void c_dscal(int n,double da,double *dx,int incx)
{
    fortran_name(dscal,DSCAL)(&n,&da,dx,&incx);
}

/************************************************************************************************/

inline void c_zscal(int n,CPX da,CPX *dx,int incx)
{
    fortran_name(zscal,ZSCAL)(&n,&da,dx,&incx);
}

/************************************************************************************************/

inline void c_dgemm(char transa, char transb, int m, int n, int k, double alpha, double *a, \
                    int lda, double *b, int ldb, double beta, double *c, int ldc)
{
    fortran_name(dgemm,DGEMM)(&transa,&transb,&m,&n,&k,&alpha,a,&lda,b,&ldb,&beta,c,&ldc);
}

/************************************************************************************************/

inline void c_dsymm(char side,char uplo, int m, int n, double alpha, double *a, int lda, \
                    double *b, int ldb, double beta, double *c, int ldc)
{
    fortran_name(dsymm,DSYMM)(&side,&uplo,&m,&n,&alpha,a,&lda,b,&ldb,&beta,c,&ldc);
}

/************************************************************************************************/

inline void c_zgemm(char transa, char transb, int m, int n, int k, CPX alpha, CPX *a, \
                    int lda, CPX *b, int ldb, CPX beta, CPX *c, int ldc)
{
    fortran_name(zgemm,ZGEMM)(&transa,&transb,&m,&n,&k,&alpha,a,&lda,b,&ldb,&beta,c,&ldc);
}

/************************************************************************************************/

inline void c_zhemm(char side, char uplo, int m, int n, CPX alpha, CPX *a, int lda, \
                    CPX *b, int ldb, CPX beta, CPX *c, int ldc)
{
    fortran_name(zhemm,ZHEMM)(&side,&uplo,&m,&n,&alpha,a,&lda,b,&ldb,&beta,c,&ldc);
}

/************************************************************************************************/

inline void c_dgemv(char transa, int m, int n, double alpha, double *a, int lda, double *x, \
                    int incx, double beta, double *y, int incy)
{
    fortran_name(dgemv,DGEMV)(&transa,&m,&n,&alpha,a,&lda,x,&incx,&beta,y,&incy);
}

/************************************************************************************************/

inline void c_zgemv(char transa, int m, int n, CPX alpha, CPX *a, int lda, CPX *x, \
                    int incx, CPX beta, CPX *y, int incy)
{
    fortran_name(zgemv,ZGEMV)(&transa,&m,&n,&alpha,a,&lda,x,&incx,&beta,y,&incy);

}

/***********************************************************************************************/
inline double c_ddot(int n, double *x, int incx, double *y, int incy)
{
    return fortran_name(ddot,DDOT)(&n,x,&incx,y,&incy);
}

/************************************************************************************************/

inline CPX c_zdotc(int n, CPX *x, int incx, CPX *y, int incy)
{
        double real,imag;

        real = c_ddot(n,(double*)x,2*incx,(double*)y,2*incy)+c_ddot(n,(double*)&x[0]+1,2*incx,(double*)&y[0]+1,2*incy);
        imag = -c_ddot(n,(double*)&x[0]+1,2*incx,(double*)y,2*incy)+c_ddot(n,(double*)x,2*incx,(double*)&y[0]+1,2*incy);

        return CPX(real,imag);

// FOR UNKNOWN REASON INTEL COMPILER SEGFAULTS WITH THAT
//        return fortran_name(zdotc,ZDOTC)(&n,x,&incx,y,&incy);
}

/************************************************************************************************/

inline void c_zcopy(int n,CPX *dx,int incx,CPX *dy,int incy)
{
    fortran_name(zcopy,ZCOPY)(&n,dx,&incx,dy,&incy);
}

/************************************************************************************************/

inline void c_zaxpy(int n, CPX alpha, CPX *x, int incx, CPX *y, int incy)
{
    fortran_name(zaxpy,ZAXPY)(&n,&alpha,x,&incx,y,&incy);
}

/************************************************************************************************/

inline double c_dasum(int n,double *dx,int incx)
{
    return fortran_name(dasum,DASUM)(&n,dx,&incx);
}

/*SAB********************************************************************************************/

inline double c_dzasum(int n,CPX *dx,int incx)
{
    return fortran_name(dzasum,DZASUM)(&n,dx,&incx);
}

/*SAB*END****************************************************************************************/

template <typename T,typename W>
inline void c_tcopy(int n,T *dx,int incx,W *dy,int incy);

template<>
inline void c_tcopy(int n,double *dx,int incx,double *dy,int incy)
{
    c_dcopy(n,dx,incx,dy,incy);
}

template<>
inline void c_tcopy(int n,CPX *dx,int incx,CPX *dy,int incy)
{
    c_zcopy(n,dx,incx,dy,incy);
}

template<>
inline void c_tcopy(int n,double *dx,int incx,CPX *dy,int incy)
{
    c_dcopy(n,dx,incx,(double*)dy,2*incy);
}

/************************************************************************************************/

template <typename T,typename W>
inline void c_taxpy(int n, T alpha, T *x, int incx, W *y, int incy);

template <>
inline void c_taxpy(int n, double alpha, double *x, int incx, double *y, int incy)
{
    c_daxpy(n,alpha,x,incx,y,incy);
}

template <>
inline void c_taxpy(int n, CPX alpha, CPX *x, int incx, CPX *y, int incy)
{
    c_zaxpy(n,alpha,x,incx,y,incy);
}

template <>
inline void c_taxpy(int n, double alpha, double *x, int incx, CPX *y, int incy)
{
    c_daxpy(n,alpha,x,incx,(double*)y,2*incy);
}

/************************************************************************************************/

template <typename T>
inline void c_tscal(int n,T da,T *dx,int incx);

template <>
inline void c_tscal(int n,double da,double *dx,int incx)
{
    c_dscal(n,da,dx,incx);
}

template <>
inline void c_tscal(int n,CPX da,CPX *dx,int incx)
{
    c_zscal(n,da,dx,incx);
}

/************************************************************************************************/

template <typename T>
inline void c_tgemm(char transa, char transb, int m, int n, int k, T alpha, T *a, \
                    int lda, T *b, int ldb, T beta, T *c, int ldc);

template <>
inline void c_tgemm(char transa, char transb, int m, int n, int k, double alpha, double *a, \
                    int lda, double *b, int ldb, double beta, double *c, int ldc)
{
    c_dgemm(transa,transb,m,n,k,alpha,a,lda,b,ldb,beta,c,ldc);
}

template <>
inline void c_tgemm(char transa, char transb, int m, int n, int k, CPX alpha, CPX *a, \
                    int lda, CPX *b, int ldb, CPX beta, CPX *c, int ldc)
{
    c_zgemm(transa,transb,m,n,k,alpha,a,lda,b,ldb,beta,c,ldc);
}

/*Lapack*****************************************************************************************/

inline void c_dgetrf(int m, int n, double *a, int lda, int *ipiv, int *info)
{
    fortran_name(dgetrf,DGETRF)(&m,&n,a,&lda,ipiv,info);
}

/************************************************************************************************/

inline void c_dgetrs(char transa, int n, int nrhs, double *a, int lda, int *ipiv, double *b, \
                   int ldb, int *info)
{
    fortran_name(dgetrs,DGETRS)(&transa,&n,&nrhs,a,&lda,ipiv,b,&ldb,info);
}

/************************************************************************************************/

inline void c_zgetrf(int m, int n, CPX *a, int lda, int *ipiv, int *info)
{
    fortran_name(zgetrf,ZGETRF)(&m,&n,a,&lda,ipiv,info);
}

/************************************************************************************************/

inline void c_zgetrs(char transa, int n, int nrhs, CPX *a, int lda, int *ipiv, CPX *b, \
                   int ldb, int *info)
{
    fortran_name(zgetrs,ZGETRS)(&transa,&n,&nrhs,a,&lda,ipiv,b,&ldb,info);
}

/************************************************************************************************/

inline void c_zgetri(int n,CPX *a,int lda,int *ipiv,CPX *work,int lwork,int *info)
{
    fortran_name(zgetri,ZGETRI)(&n,a,&lda,ipiv,work,&lwork,info);
}

/************************************************************************************************/

inline void c_dgeev(char jobvl, char jobvr, int n, double *a, int lda, double *wr, double *wi, \
                    double *vl, int ldvl, double *vr, int ldvr, double *work, int lwork, \
                    int *info)
{
    fortran_name(dgeev,DGEEV)(&jobvl,&jobvr,&n,a,&lda,wr,wi,vl,&ldvl,vr,&ldvr,work,&lwork,info);
}

/************************************************************************************************/

inline void c_dsyev(char JOBZ,char UPLO,int N,double *A,int LDA,double *W,double *WORK,int LWORK,\
                    int *INFO)
{
  fortran_name(dsyev,DSYEV)(&JOBZ,&UPLO,&N,A,&LDA,W,WORK,&LWORK,INFO);
}

/************************************************************************************************/

inline void c_dggev(char jobvl, char jobvr, int n, double *a, int lda, double *b, int ldb, \
                    double *alphar, double *alphai, double *beta, double *vl, int ldvl, \
                    double *vr, int ldvr, double *work, int lwork, int *info)
{
    fortran_name(dggev,DGGEV)(&jobvl,&jobvr,&n,a,&lda,b,&ldb,alphar,alphai,beta,vl,&ldvl,vr,\
                              &ldvr,work,&lwork,info);
}

/************************************************************************************************/

inline void c_zggev(char jobvl, char jobvr, int n, CPX *a, int lda, CPX *b, int ldb, \
                    CPX *alpha, CPX *beta, CPX *vl, int ldvl, \
                    CPX *vr, int ldvr, CPX *work, int lwork, double *rwork, int *info)
{
    fortran_name(zggev,ZGGEV)(&jobvl,&jobvr,&n,a,&lda,b,&ldb,alpha,beta,vl,&ldvl,vr,\
                              &ldvr,work,&lwork,rwork,info);
}

/************************************************************************************************/

inline void c_zgeev(char jobvl, char jobvr, int n, CPX *a, int lda, CPX *w, CPX *vl, int ldvl, \
                    CPX *vr, int ldvr, CPX *work, int lwork, double *rwork, int *info)
{
    fortran_name(zgeev,ZGEEV)(&jobvl,&jobvr,&n,a,&lda,w,vl,&ldvl,vr,&ldvr,work,&lwork,rwork,info);
}

/************************************************************************************************/

inline void c_zheev(char jobvl, char uplo, int n, CPX *a, int lda, double *w, CPX *work, \
		    int lwork, double *rwork, int *info)
{
    fortran_name(zheev,ZHEEV)(&jobvl,&uplo,&n,a,&lda,w,work,&lwork,rwork,info);
}

/************************************************************************************************/

inline void c_dgetri(int n,double *a,int lda,int *ipiv,double *work,int lwork,int *info)
{
    fortran_name(dgetri,DGETRI)(&n,a,&lda,ipiv,work,&lwork,info);
}

/************************************************************************************************/

inline void c_dsytri(char uplo,int n,double *a,int lda,int *ipiv,double *work,int *info)
{
    fortran_name(dsytri,DSYTRI)(&uplo,&n,a,&lda,ipiv,work,info);
}

/************************************************************************************************/

inline void c_zhetrf(char uplo,int n,CPX *a,int lda,int *ipiv,CPX *work,int lwork,int *info)
{
    fortran_name(zhetrf,ZHETRF)(&uplo,&n,a,&lda,ipiv,work,&lwork,info);
}

/************************************************************************************************/

inline void c_zhetri(char uplo,int n,CPX *a,int lda,int *ipiv,CPX *work,int *info)
{
    fortran_name(zhetri,ZHETRI)(&uplo,&n,a,&lda,ipiv,work,info);
}

/************************************************************************************************/

inline void c_zhetrs(char uplo,int n,int nrhs,CPX *a,int lda,int *ipiv,CPX *b,int ldb,\
                     int *info)
{
    fortran_name(zhetrs,ZHETRS)(&uplo,&n,&nrhs,a,&lda,ipiv,b,&ldb,info);
}

/************************************************************************************************/

inline void c_dsysv(char uplo,int n,int nrhs,double *a,int lda,int *ipiv,double *b,int ldb,\
                    double *work,int lwork,int *info)
{
    fortran_name(dsysv,DSYSV)(&uplo,&n,&nrhs,a,&lda,ipiv,b,&ldb,work,&lwork,info);
}

/************************************************************************************************/

inline void c_dsytrf(char uplo,int n,double *a,int lda,int *ipiv,double *work,int lwork,int *info)
{
    fortran_name(dsytrf,DSYTRF)(&uplo,&n,a,&lda,ipiv,work,&lwork,info);
}

/************************************************************************************************/

inline void c_dsytrs(char uplo,int n,int nrhs,double *a,int lda,int *ipiv,double *b,int ldb,\
                     int *info)
{
    fortran_name(dsytrs,DSYTRS)(&uplo,&n,&nrhs,a,&lda,ipiv,b,&ldb,info);
}

/************************************************************************************************/

inline void c_dstebz(char *range,char *order,int *iter,double *vl,double *vu,int *il,int *iu,\
                     double *abstol,double *diag,double *offd,int *neval,int *nsplit,\
                     double *eval,int *iblock,int *isplit,double *work,int *iwork,int *info)
{
    fortran_name(dstebz,DSTEBZ)(range,order,iter,vl,vu,il,iu,abstol,diag,offd,neval,nsplit,eval,\
	                            iblock,isplit,work,iwork,info);
}

/************************************************************************************************/

inline void c_zlarnv(int *idist,int *iseed,int n,CPX *x)
{
	fortran_name(zlarnv,ZLARNV)(idist,iseed,&n,x);
}

/*SAB********************************************************************************************/

inline void c_dsygv(int itype,char jobz,char uplo,int n,double *a,int lda,double *b,int ldb,\
                    double *w,double *work,int lwork,int *info)
{
    fortran_name(dsygv,DSYGV)(&itype,&jobz,&uplo,&n,a,&lda,b,&ldb,w,work,&lwork,info);
}

/************************************************************************************************/

inline void c_zhegv(int itype,char jobz,char uplo,int n,CPX *a,int lda,CPX *b,int ldb,\
                    double *w,CPX *work,int lwork,double *rwork,int *info)
{
    fortran_name(zhegv,ZHEGV)(&itype,&jobz,&uplo,&n,a,&lda,b,&ldb,w,work,&lwork,rwork,info);
}

/************************************************************************************************/

inline void c_zhegvx(int itype,char jobz,char range,char uplo,int n,CPX *a,int lda,\
                     CPX *b,int ldb, double vl, double vu, int il,int iu,double abstol,\
                     int *m,double *w,CPX *z,int ldz,CPX *work,int lwork,double *rwork,\
                     int *iwork,int *ifail,int *info)
{
    fortran_name(zhegvx,ZHEGVX)(&itype,&jobz,&range,&uplo,&n,a,&lda,b,&ldb,&vl,&vu,&il,&iu,&abstol,\
                              m,w,z,&ldz,work,&lwork,rwork,iwork,ifail,info);
}

/************************************************************************************************/

inline void c_dgesdd(char jobz,int m,int n,double *a,int lda,double *s,double *u,int ldu,\
                     double *vt,int ldvt,double *work,int lwork,int *iwork,int *info)
{
    fortran_name(dgesdd,DGESDD)(&jobz,&m,&n,a,&lda,s,u,&ldu,vt,&ldvt,work,&lwork,iwork,info);
}

/************************************************************************************************/

inline void c_zgesdd(char jobz,int m,int n,CPX *a,int lda,double *s,CPX *u,int ldu,\
                     CPX *vt,int ldvt,CPX *work,int lwork,double *rwork,int *iwork,int *info)
{
    fortran_name(zgesdd,ZGESDD)(&jobz,&m,&n,a,&lda,s,u,&ldu,vt,&ldvt,work,&lwork,rwork,iwork,info);
}

/************************************************************************************************/

inline double c_dlamch(char cmach)
{
    return fortran_name(dlamch,DLAMCH)(&cmach);
}

/************************************************************************************************/

inline void c_dlasrt(char id,int n,double* d,int *info)
{
    fortran_name(dlasrt,DLASRT)(&id,&n,d,info);
}

/*SAB*END****************************************************************************************/

/*MMC********************************************************************************************/

inline void c_dsterf(int N, double *D, double *E, int *INFO)
{
  fortran_name(dsterf,DSTERF)(&N, D, E, INFO);
}

inline void c_dsteqr(char COMPZ, int N, double *D, double *E, double *Z, 
                     int LDZ, double *WORK, int *INFO)
{
  fortran_name(dsteqr,DSTEQR)(&COMPZ, &N, D, E, Z, &LDZ, WORK, INFO);
}

inline void c_dlacpy(char UPLO, int M, int N, double* A, int LDA, double* B,
                     int LDB)
{
  fortran_name(dlacpy,DLACPY)(&UPLO, &M, &N, A, &LDA, B, &LDB);
}

inline void c_zlacpy(char UPLO, int M, int N, CPX* A, int LDA, CPX* B, int LDB)
{
  fortran_name(zlacpy,ZLACPY)(&UPLO, &M, &N, A, &LDA, B, &LDB);
}

/*MMC*END***************************************************************************************/

#ifdef HAVE_SPLITSOLVE
template <typename T>
inline void tgemm_dev(void *handle,char transa, char transb, int m, int n, int k, T alpha, T *a,\
		      int lda, T *b, int ldb, T beta, T *c, int ldc);

template <>
inline void tgemm_dev(void *handle,char transa, char transb, int m, int n, int k, double alpha,\
		      double *a, int lda, double *b, int ldb, double beta, double *c, int ldc)
{
    dgemm_on_dev(handle,transa,transb,m,n,k,alpha,a,lda,b,ldb,beta,c,ldc);
}

template <>
inline void tgemm_dev(void *handle,char transa, char transb, int m, int n, int k, CPX alpha,\
		      CPX *a, int lda, CPX *b, int ldb, CPX beta, CPX *c, int ldc)
{
    zgemm_on_dev(handle,transa,transb,m,n,k,alpha,a,lda,b,ldb,beta,c,ldc);
}
#endif

/*magma*****************************************************************************************/

#ifdef HAVE_SPLITSOLVE
template <typename T>
inline void tgetrf_dev(int m,int n,T *a,int lda,int *ipiv,int *info);

template <>
inline void tgetrf_dev(int m,int n,double *a,int lda,int *ipiv,int *info)
{
    magma_dgetrf_gpu(m,n,a,lda,ipiv,info);
}

template <>
inline void tgetrf_dev(int m,int n,CPX *a,int lda,int *ipiv,int *info)
{
    magma_zgetrf_gpu(m,n,(magmaDoubleComplex_ptr)a,lda,ipiv,info);
}

/************************************************************************************************/

template <typename T>
inline void tgetrs_dev(char transa,int n,int nrhs,T *a,int lda,int *ipiv,T *b,int ldb,int *info);

template <>
inline void tgetrs_dev(char transa,int n,int nrhs,double *a,int lda,int *ipiv,double *b,int ldb,\
		       int *info)
{
    magma_trans_t magma_opa;

    if(transa=='N'){
        magma_opa = MagmaNoTrans; 
    }
    if(transa=='T'){
        magma_opa = MagmaTrans;
    }

    magma_dgetrs_gpu(magma_opa,n,nrhs,a,lda,ipiv,b,ldb,info);
}

template <>
inline void tgetrs_dev(char transa,int n,int nrhs,CPX *a,int lda,int *ipiv,CPX *b,int ldb,\
		       int *info)
{
    magma_trans_t magma_opa;

    if(transa=='N'){
        magma_opa = MagmaNoTrans; 
    }
    if(transa=='T'){
        magma_opa = MagmaTrans;
    }

    magma_zgetrs_gpu(magma_opa,n,nrhs,(magmaDoubleComplex_ptr)a,lda,ipiv,\
		     (magmaDoubleComplex_ptr)b,ldb,info);
}

/************************************************************************************************/

template <typename T>
inline void tgesv_dev(int n,int nrhs,T *a,int lda,int *ipiv,T *b,int ldb,int type,int *info);

template <>
inline void tgesv_dev(int n,int nrhs,double *a,int lda,int *ipiv,double *b,int ldb,int type,int *info)
{
    if(type){
	magma_dgesv_nopiv_gpu(n,nrhs,a,lda,b,ldb,info);  
    }else{
      /*                                                                                                                   
        magma_dgesv_nopiv_gpu(n,nrhs,a,lda,b,ldb,info);                                                                    
      */
        magma_dsysv_nopiv_gpu(MagmaLower,n,nrhs,a,lda,b,ldb,info);
    }
}

template <>
inline void tgesv_dev(int n,int nrhs,CPX *a,int lda,int *ipiv,CPX *b,int ldb,int type,int *info)
{

    if(type){
	magma_zgesv_nopiv_gpu(n,nrhs,(magmaDoubleComplex_ptr)a,lda,(magmaDoubleComplex_ptr)b,\
			      ldb,info);
    }else{
      /*                                                                                                 
        magma_zgesv_nopiv_gpu(n,nrhs,(magmaDoubleComplex_ptr)a,lda,(magmaDoubleComplex_ptr)b,\
			      ldb,info);                                                                                         
      */
        magma_zhesv_nopiv_gpu(MagmaLower,n,nrhs,(magmaDoubleComplex_ptr)a,lda,(magmaDoubleComplex_ptr)b,\
                              ldb,info);
    }
}

/************************************************************************************************/

template <typename T>
inline void tgetri_dev(int n,T *a,int lda,int *ipiv,T *work,int lwork,int *info);

template <>
inline void tgetri_dev(int n,double *a,int lda,int *ipiv,double *work,int lwork,int *info)
{
    magma_dgetri_gpu(n,a,lda,ipiv,work,lwork,info);
}

template <>
inline void tgetri_dev(int n,CPX *a,int lda,int *ipiv,CPX *work,int lwork,int *info)
{
    magma_zgetri_gpu(n,(magmaDoubleComplex_ptr)a,lda,ipiv,(magmaDoubleComplex_ptr)work,lwork,info);
}

/************************************************************************************************/

inline void zgetrf_nopiv_dev(int m,int n,CPX *a,int lda,int *info)
{
    magma_zgetrf_nopiv_gpu(m,n,(magmaDoubleComplex_ptr)a,lda,info);
}

/************************************************************************************************/

inline void zgetri_dev(int n,CPX *a,int lda,int *ipiv,CPX *work,int lwork,int *info)
{
    magma_zgetri_gpu(n,(magmaDoubleComplex_ptr)a,lda,ipiv,(magmaDoubleComplex_ptr)work,lwork,info);
}

/************************************************************************************************/

inline void zgetrs_nopiv_dev(char transa,int n,int nrhs,CPX *a,int lda,CPX *b,int ldb,int *info)
{
    magma_trans_t magma_opa;

    if(transa=='N'){
        magma_opa = MagmaNoTrans; 
    }
    if(transa=='T'){
        magma_opa = MagmaTrans;
    }

    magma_zgetrs_nopiv_gpu(magma_opa,n,nrhs,(magmaDoubleComplex_ptr)a,lda,\
			   (magmaDoubleComplex_ptr)b,ldb,info);
}

/************************************************************************************************/

#endif

/************************************************************************************************/

#ifdef HAVE_SPLITSOLVE
template <typename T>
inline void copy_csr_to_device(int size,int n_nonzeros,int *hedge_i,int *hindex_j,T *hnnz,\
			       int *dedge_i,int *dindex_j,T *dnnz);

template <>
inline void copy_csr_to_device(int size,int n_nonzeros,int *hedge_i,int *hindex_j,double *hnnz,\
			       int *dedge_i,int *dindex_j,double *dnnz)
{
    d_copy_csr_to_device(size,n_nonzeros,hedge_i,hindex_j,hnnz,dedge_i,dindex_j,dnnz);
}

template <>
inline void copy_csr_to_device(int size,int n_nonzeros,int *hedge_i,int *hindex_j,CPX *hnnz,\
			       int *dedge_i,int *dindex_j,CPX *dnnz)
{
    z_copy_csr_to_device(size,n_nonzeros,hedge_i,hindex_j,hnnz,dedge_i,dindex_j,dnnz);
}

/************************************************************************************************/

template <typename T>
inline void init_var_on_dev(T *var,int N,cudaStream_t stream);

template <>
inline void init_var_on_dev(double *var,int N,cudaStream_t stream){
    d_init_var_on_dev(var,N,stream);
}

template <>
inline void init_var_on_dev(CPX *var,int N,cudaStream_t stream){
    z_init_var_on_dev(var,N,stream);
}

/************************************************************************************************/

template <typename T>
inline void init_eye_on_dev(T *var,int N,cudaStream_t stream);

template <>
inline void init_eye_on_dev(double *var,int N,cudaStream_t stream){
    d_init_eye_on_dev(var,N,stream);
}

template <>
inline void init_eye_on_dev(CPX *var,int N,cudaStream_t stream){
    z_init_eye_on_dev(var,N,stream);
}

/************************************************************************************************/

template <typename T>
inline void csr_mult_f(void *handle,int m,int n,int k,int n_nonzeros,int *Aedge_i,int *Aindex_j,\
		       T *Annz,T alpha,T *B,T beta,T *C);

template <>
inline void csr_mult_f(void *handle,int m,int n,int k,int n_nonzeros,int *Aedge_i,int *Aindex_j,\
		       double *Annz,double alpha,double *B,double beta,double *C)
{
    d_csr_mult_f(handle,m,n,k,n_nonzeros,Aedge_i,Aindex_j,Annz,alpha,B,beta,C);
}

template <>
inline void csr_mult_f(void *handle,int m,int n,int k,int n_nonzeros,int *Aedge_i,int *Aindex_j,\
		       CPX *Annz,CPX alpha,CPX *B,CPX beta,CPX *C)
{
    z_csr_mult_f(handle,m,n,k,n_nonzeros,Aedge_i,Aindex_j,Annz,alpha,B,beta,C);
}

/************************************************************************************************/

template <typename T>
inline void transpose_matrix(T *odata,T *idata,int size_x,int size_y);

template <>
inline void transpose_matrix(double *odata,double *idata,int size_x,int size_y)
{
    d_transpose_matrix(odata,idata,size_x,size_y);
}

template <>
inline void transpose_matrix(CPX *odata,CPX *idata,int size_x,int size_y)
{
    z_transpose_matrix(odata,idata,size_x,size_y);
}

#endif

/************************************************************************************************/

}

#endif




