/* @HEADER@ */
/* ***********************************************************************
// 
//           TSFExtended: Trilinos Solver Framework Extended
//                 Copyright (2004) Sandia Corporation
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
// **********************************************************************/
 /* @HEADER@ */


#ifndef TSFOPERATORBUILDER_HPP
#define TSFOPERATORBUILDER_HPP

#include "TSFLinearOperatorDecl.hpp"
#include "TSFLinearCombinationDecl.hpp"
#include "TSFVectorType.hpp"
#include "TSFVectorSpaceDecl.hpp"
#include "Teuchos_Array.hpp"
#include "Teuchos_GlobalMPISession.hpp"


using namespace TSFExtended;
using namespace Teuchos;


namespace TSFExtended
{
  /** Base class for building test operators */
  template <class Scalar>
  class OperatorBuilder
  {
  public:
    /** */
    OperatorBuilder(int nLocal, const VectorType<Scalar>& vecType);
    /** */
    OperatorBuilder(int nLocalDomain, int nLocalRange,
                    const VectorType<Scalar>& vecType);
    /** */
    OperatorBuilder(const VectorSpace<Scalar>& domain,
                    const VectorSpace<Scalar>& range,
                    const VectorType<Scalar>& vecType);
    /** */
    virtual ~OperatorBuilder(){;}

    /** */
    const VectorType<Scalar>& vecType() const {return vecType_;}

    /** */
    const VectorSpace<Scalar>& domain() const {return domain_;}

    /** */
    const VectorSpace<Scalar>& range() const {return range_;}

    /** */
    virtual LinearOperator<Scalar> getOp() const = 0 ; 

  protected:

  private:
    VectorType<Scalar> vecType_;

    VectorSpace<Scalar> domain_;

    VectorSpace<Scalar> range_;
  };

  template <class Scalar> 
  inline OperatorBuilder<Scalar>
  ::OperatorBuilder(int nLocalRows, const VectorType<Scalar>& vecType)
    : vecType_(vecType), domain_(), range_()
  {
    range_ = vecType_.createEvenlyPartitionedSpace(MPIComm::world(), nLocalRows);
    domain_ = range_;
  }

  template <class Scalar> 
  inline OperatorBuilder<Scalar>
  ::OperatorBuilder(int nLocalDomain, int nLocalRange,
                    const VectorType<Scalar>& vecType)
    : vecType_(vecType), domain_(), range_()
  {
    range_ = vecType_.createEvenlyPartitionedSpace(MPIComm::world(), nLocalRange);
    domain_ = vecType_.createEvenlyPartitionedSpace(MPIComm::world(), nLocalDomain);
  }

  

  template <class Scalar> 
  inline OperatorBuilder<Scalar>
  ::OperatorBuilder(const VectorSpace<Scalar>& domain,
                    const VectorSpace<Scalar>& range,
                    const VectorType<Scalar>& vecType)
    : vecType_(vecType), domain_(domain), range_(range)
  {}

}

#endif
