#include "blas_extended.h"
#include "blas_extended_private.h"
void BLAS_zgbmv_d_d(enum blas_order_type order, enum blas_trans_type trans,
		    int m, int n, int kl, int ku, const void *alpha,
		    const double *a, int lda, const double *x, int incx,
		    const void *beta, void *y, int incy)

/*           
 * Purpose
 * =======
 *
 *  gbmv computes y = alpha * A * x + beta * y, where 
 *
 *  A is a m x n banded matrix
 *  x is a n x 1 vector
 *  y is a m x 1 vector
 *  alpha and beta are scalars 
 *
 *   
 * Arguments
 * =========
 *
 * order        (input) blas_order_type
 *              Order of AP; row or column major
 *
 * trans        (input) blas_trans_type
 *              Transpose of AB; no trans, 
 *              trans, or conjugate trans
 *
 * m            (input) int
 *              Dimension of AB
 *
 * n            (input) int
 *              Dimension of AB and the length of vector x
 *
 * kl           (input) int 
 *              Number of lower diagnols of AB
 *
 * ku           (input) int
 *              Number of upper diagnols of AB
 *
 * alpha        (input) const void*
 *              
 * AB           (input) double*
 *
 * lda          (input) int 
 *              Leading dimension of AB
 *              lda >= ku + kl + 1
 *
 * x            (input) double*
 * 
 * incx         (input) int
 *              The stride for vector x.
 *
 * beta         (input) const void*
 *
 * y            (input/output) void*
 *
 * incy         (input) int
 *              The stride for vector y.
 * 
 *
 * LOCAL VARIABLES 
 * ===============
 * 
 *  As an example, these variables are described on the mxn, column 
 *  major, banded matrix described in section 2.2.3 of the specification  
 *
 *  astart      indexes first element in A where computation begins
 *
 *  incai1      indexes first element in row where row is less than lbound
 * 
 *  incai2      indexes first element in row where row exceeds lbound
 *   
 *  lbound      denotes the number of rows before  first element shifts 
 *
 *  rbound      denotes the columns where there is blank space
 *   
 *  ra          index of the rightmost element for a given row
 *  
 *  la          index of leftmost  elements for a given row
 *
 *  ra - la     width of a row
 *
 *                        rbound 
 *            la   ra    ____|_____ 
 *             |    |   |          |
 *         |  a00  a01   *    *   *
 * lbound -|  a10  a11  a12   *   *
 *         |  a20  a21  a22  a23  *
 *             *   a31  a32  a33 a34
 *             *    *   a42  a43 a44
 *
 *  Varations on order and transpose have been implemented by modifying these
 *  local variables. 
 *
 */
{
  static const char routine_name[] = "BLAS_zgbmv_d_d";

  int ky, iy, kx, jx, j, i, rbound, lbound, ra, la, lenx, leny;
  int incaij, aij, incai1, incai2, astart, ai;
  double *y_i = (double *) y;
  const double *a_i = a;
  const double *x_i = x;
  double *alpha_i = (double *) alpha;
  double *beta_i = (double *) beta;
  double tmp1[2];
  double tmp2[2];
  double result[2];
  double sum;
  double prod;
  double a_elem;
  double x_elem;
  double y_elem[2];


  if (((trans != blas_no_trans) &&
       (trans != blas_trans) &&
       (trans != blas_conj_trans)) ||
      (m < 0) || (n < 0) ||
      (kl < 0) || (kl >= m) ||
      (ku < 0) || (ku >= n) ||
      (lda < (kl + ku + 1)) ||
      (incx == 0) || (incy == 0) ||
      (order == blas_rowmajor && lda < n) ||
      (order == blas_colmajor && lda < m)) {
    BLAS_error(routine_name, 0, 0, NULL);
  }

  if ((m == 0) || (n == 0) ||
      (((alpha_i[0] == 0.0 && alpha_i[1] == 0.0)
	&& ((beta_i[0] == 1.0 && beta_i[1] == 0.0)))))
    return;

  if (trans == blas_no_trans) {
    lenx = n;
    leny = m;
  } else {			/* change back */
    lenx = m;
    leny = n;
  }

  if (incx < 0) {
    kx = -(lenx - 1) * incx;
  } else {
    kx = 0;
  }

  if (incy < 0) {
    ky = -(leny - 1) * incy;
  } else {
    ky = 0;
  }



  /* if alpha = 0, return y = y*beta */
  if ((order == blas_colmajor) && (trans == blas_no_trans)) {
    astart = ku;
    incai1 = 1;
    incai2 = lda;
    incaij = lda - 1;
    lbound = kl;
    rbound = n - ku - 1;
    ra = ku;
  } else if ((order == blas_colmajor) && (trans != blas_no_trans)) {
    astart = ku;
    incai1 = lda - 1;
    incai2 = lda;
    incaij = 1;
    lbound = ku;
    rbound = m - kl - 1;
    ra = kl;
  } else if ((order == blas_rowmajor) && (trans == blas_no_trans)) {
    astart = kl;
    incai1 = lda - 1;
    incai2 = lda;
    incaij = 1;
    lbound = kl;
    rbound = n - ku - 1;
    ra = ku;
  } else {			/* rowmajor and blas_trans */
    astart = kl;
    incai1 = 1;
    incai2 = lda;
    incaij = lda - 1;
    lbound = ku;
    rbound = m - kl - 1;
    ra = kl;
  }

  incy *= 2;




  ky *= 2;


  la = 0;
  ai = astart;
  iy = ky;
  for (i = 0; i < leny; i++) {
    sum = 0.0;
    aij = ai;
    jx = kx;

    for (j = ra - la; j >= 0; j--) {
      x_elem = x_i[jx];
      a_elem = a_i[aij];
      prod = x_elem * a_elem;
      sum = sum + prod;
      aij += incaij;
      jx += incx;
    }


    {
      tmp1[0] = alpha_i[0] * sum;
      tmp1[1] = alpha_i[1] * sum;
    }
    y_elem[0] = y_i[iy];
    y_elem[1] = y_i[iy + 1];
    {
      tmp2[0] =
	(double) beta_i[0] * y_elem[0] - (double) beta_i[1] * y_elem[1];
      tmp2[1] =
	(double) beta_i[0] * y_elem[1] + (double) beta_i[1] * y_elem[0];
    }
    result[0] = tmp1[0] + tmp2[0];
    result[1] = tmp1[1] + tmp2[1];
    y_i[iy] = result[0];
    y_i[iy + 1] = result[1];
    iy += incy;
    if (i >= lbound) {
      kx += incx;
      ai += incai2;
      la++;
    } else {
      ai += incai1;
    }
    if (i < rbound) {
      ra++;
    }
  }



}				/* end GEMV_NAME(z, d, d, ) */
