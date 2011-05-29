// @HEADER
// ***********************************************************************
// 
//                 TriUtils: Trilinos Utilities Package
//                 Copyright (2001) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#include "Trilinos_Util.h"

double Trilinos_Util_svbrres (int m, int n, int m_blk,
		double *val, int *indx, int *bindx, int *rpntr,
		int *cpntr, int *bpntrb, int *bpntre,
		double *x, double *b)
{
    int i;
    double norm_tmp = 0.0, norm_b = 0.0;
    double scaled_res_norm, res_norm, *tmp, max_norm = 0.0;
    SPBLASMAT  A;



/*     Computes the residual

                      res = || b - A*x ||

       where x and b are vectors and A is a sparse matrix stored
       in MSR format. */

/*     -------------------------- 
       First executable statement 
       -------------------------- */
    /* Create sparse matrix handle */
    Trilinos_Util_duscr_vbr(m_blk, val, indx, bindx, rpntr, cpntr, bpntrb, bpntre, &A);
    

    /* Create tmp workspace, set to b */

    tmp = (double *) calloc(m,sizeof(double));

    for (i = 0; i < m; i++) tmp[i] = b[i];

    /* Call DUSMM to compute residual (in tmp) */

    Trilinos_Util_dusmm(m_blk, 1, n, -1.0, &A, x, m, 1.0, tmp, m);

    Trilinos_Util_dusds_vbr(&A);

    for (i = 0; i <m ; i++) 
      {
	max_norm = Trilinos_Util_max(fabs(tmp[i]),max_norm);
	norm_tmp += tmp[i]*tmp[i];
	norm_b += b[i]*b[i];
      }
   
    res_norm = sqrt(norm_tmp);
    scaled_res_norm = res_norm/sqrt(norm_b);
    printf("\n\nMax norm of residual        = %12.4g\n",max_norm);
    printf(    "Two norm of residual        = %12.4g\n",res_norm);
    if (norm_b > 1.0E-7) 
      {
	scaled_res_norm = res_norm/sqrt(norm_b);
	printf(    "Scaled two norm of residual = %12.4g\n",scaled_res_norm);
      }
    free((void *) tmp);

    return(res_norm);

} /* svbrres */
