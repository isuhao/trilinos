// @HEADER
// ***********************************************************************
// 
// Moocho: Multi-functional Object-Oriented arCHitecture for Optimization
//                  Copyright (2003) Sandia Corporation
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
// Questions? Contact Roscoe A. Bartlett (rabartl@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef MATRIX_WITH_OP_GET_GMS_TRI_H
#define MATRIX_WITH_OP_GET_GMS_TRI_H

#include "AbstractLinAlgPack_Types.hpp"
#include "AbstractLinAlgPack_MatrixOp.hpp"
#include "DenseLinAlgPack_DMatrixAsTriSym.hpp"
#include "Teuchos_dyn_cast.hpp"

namespace AbstractLinAlgPack {

/** \brief Mix-in interface that allows the extraction of a const <tt>DenseLinAlgPack::DMatrixSliceTri</tt>
 * view of an non-singular abstract matrix.
 *
 * This interface is ment to be used by <tt>MatrixOp</tt> objects
 * that store all of their matrix elements in the local address space or can easily
 * access all of the elements from this process.
 *
 * Subclasses that store a BLAS compatible triangular dense matrix can implement
 * these methods without any dynamic memory allocations.  There is no default
 * implementation for these methods so subclasses that derive from this interface
 * must implement these methods.
 *
 * These methods should never be called directly.  Instead, use the helper
 * class type <tt>MatrixDenseTriEncap</tt>.
 */
class MatrixOpGetGMSTri
  : virtual public AbstractLinAlgPack::MatrixOp // doxygen needs full name
{
public:

  /** \brief Get a const view of the symmetric abstract matrix in the form <tt>DenseLinAlgPack::DMatrixSliceTri</tt>.
   *
   * @return On ouput, \c return will be initialized to point to storage to the symmetric dense
   *  matrix elements.
   * The output from this function <tt>tri_gms_view = this->get_tri_gms_view()</tt> must be passed to
   * <tt>this->free_tri_gms_view(gms)</tt> to free any memory that may have been allocated.
   * After <tt>this->free_gms_view(gms_view)</tt> is called, \c gms_view must not be used any longer!
   *
   * Postconditions:<ul>
   * <li> <tt>return.rows() == this->rows()</tt>
   * <li> <tt>return.cols() == this->cols()</tt>
   * </ul>
   *
   * Warning!  If a subclass overrides this method, it must also override \c free_tri_gms_view().
   */
  virtual const DenseLinAlgPack::DMatrixSliceTri get_tri_gms_view() const = 0;

  /** \brief Free a view of a symmetric dense matrix initialized from <tt>get_tri_gms_view()>/tt>.
   *
   * @param  tri_gms_view
   *              [in/out] On input, \c tri_gms_view must have been initialized from \c this->get_tri_gms_view().
   *              On output, \c tri_gms_view will become invalid and must not be used.
   *
   * Preconditions:<ul>
   * <li> \c tri_gms_view must have been initialized by \c this->get_tri_gms_view)
   * </ul>
   *
   * Postconditions:<ul>
   * <li> \c tri_gms_view becomes invalid and must not be used any longer!
   * </ul>
   */
  virtual void free_tri_gms_view(const DenseLinAlgPack::DMatrixSliceTri* tri_gms_view) const = 0;

}; // end class MatrixOpGetGMSTri

/** \brief Helper class type that simplifies the usage of the <tt>MatrixOpGetGMSTri</tt> interface for clients.
 *
 * This takes care of worrying about if the <tt>MatrixOpGetGMSTri</tt> interface is supported or not
 * and remembering to free the <tt>DenseLinAlgPack::DMatrixSliceTri</tt> view properly.
 *
 * This class is only to be used on the stack as an automatic variable.  For example, to extract a
 * <tt>DenseLinAlgPack::DMatrixSliceTri</tt> view of an abstract vector and use it to call another function
 * one could write a function like:
 \code
 void call_func(const MatrixOpGetGMSTri& mat_in ) {
     func( MatrixDenseTriEncap(mat_in)() );
 }
 \endcode
 * In the above code, if the underlying <tt>MatrixOpGetGMSTri</tt> object does not have to
 * perform any dynamic memory allocations and copy in the method
 * <tt>MatrixOpGetGMSTri::get_tri_gms_view()</tt> then the above code will only have a constant
 * time overhead.
 */
class MatrixDenseTriEncap {
public:

  /** \brief Construct a <tt>DenseLinAlgPack::DMatrixSliceTri</tt> view from a <tt>MatrixOpGetGMSTri</tt> object.
   */
  MatrixDenseTriEncap( const MatrixOpGetGMSTri&  mat_get );
  /** \brief Construct a <tt>DenseLinAlgPack::DMatrixSliceTri</tt> view from a <tt>MatrixOp</tt> object.
   *
   * If <tt>dynamic_cast<const MatrixOpGetGMSTri*>(&mat) == NULL</tt> then a ???
   * exception is thrown.
   */
  MatrixDenseTriEncap( const MatrixOp& mat );
  /// Frees the <tt>DenseLinAlgPack::DMatrixSliceTri</tt> view.
  ~MatrixDenseTriEncap();
  /// Returns a constant view of the <tt>DenseLinAlgPack::DMatrixSliceTri</tt> view.
  const DenseLinAlgPack::DMatrixSliceTri operator()() const;

private:

  const MatrixOpGetGMSTri     &mat_get_;
  const DenseLinAlgPack::DMatrixSliceTri       tri_gms_view_;
  MatrixDenseTriEncap();                                      // Not defined and not to be called!
  MatrixDenseTriEncap(const MatrixDenseTriEncap&);               // ""
  MatrixDenseTriEncap& operator=(const MatrixDenseTriEncap&);    // ""

}; // end class MatrixDenseTriEncap

// ///////////////////////////////////////////
// Inline members

// MatrixDenseTriEncap

inline
MatrixDenseTriEncap::MatrixDenseTriEncap( const MatrixOpGetGMSTri&  mat_get )
  :mat_get_(mat_get)
  ,tri_gms_view_(mat_get_.get_tri_gms_view())
{}

inline
MatrixDenseTriEncap::MatrixDenseTriEncap( const MatrixOp& mat )
  :mat_get_(Teuchos::dyn_cast<const MatrixOpGetGMSTri>(mat))
  ,tri_gms_view_(mat_get_.get_tri_gms_view())
{}

inline
MatrixDenseTriEncap::~MatrixDenseTriEncap()
{
  mat_get_.free_tri_gms_view(&tri_gms_view_);
}

inline
const DenseLinAlgPack::DMatrixSliceTri MatrixDenseTriEncap::operator()() const
{
  return tri_gms_view_;
}

} // end namespace AbstractLinAlgPack

#endif // MATRIX_WITH_OP_GET_GMS_TRI_H
