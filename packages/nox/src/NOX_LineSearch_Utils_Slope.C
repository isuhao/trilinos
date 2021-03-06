// $Id$ 
// $Source$ 

//@HEADER
// ************************************************************************
// 
//            NOX: An Object-Oriented Nonlinear Solver Package
//                 Copyright (2002) Sandia Corporation
// 
//            LOCA: Library of Continuation Algorithms Package
//                 Copyright (2005) Sandia Corporation
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
// 
// Questions? Contact Roger Pawlowski (rppawlo@sandia.gov) or 
// Eric Phipps (etphipp@sandia.gov), Sandia National Laboratories.
// ************************************************************************
//  CVS Information
//  $Source$
//  $Author$
//  $Date$
//  $Revision$
// ************************************************************************
//@HEADER

#include "NOX_LineSearch_Utils_Slope.H"
#include "NOX_Abstract_Vector.H"
#include "NOX_Abstract_Group.H"
#include "NOX_GlobalData.H"

NOX::LineSearch::Utils::Slope::
Slope(const Teuchos::RCP<NOX::GlobalData>& gd) :
  utils(*(gd->getUtils()))
{

}

NOX::LineSearch::Utils::Slope::~Slope()
{

}

void NOX::LineSearch::Utils::Slope::
reset(const Teuchos::RCP<NOX::GlobalData>& gd)
{
  utils = *(gd->getUtils());
}

double NOX::LineSearch::Utils::Slope::
computeSlope(const Abstract::Vector& dir, const Abstract::Group& grp) 
{
   if (grp.isGradient()) 
     return(dir.innerProduct(grp.getGradient()));

  // Allocate space for vecPtr if necessary
   if (Teuchos::is_null(vecPtr)) 
     vecPtr = dir.clone(ShapeCopy);

  // v = J * dir
  NOX::Abstract::Group::ReturnType status = grp.applyJacobian(dir,*vecPtr);
  
  if (status != NOX::Abstract::Group::Ok) 
  {
    utils.out() << "NOX::LineSearch::Utils::Slope::computeSlope -  Unable to apply Jacobian!" << endl;
    throw "NOX Error";
  }

  // Check that F exists
  if (!grp.isF()) 
  {
    utils.out() << "NOX::LineSearch::Utils::Slope::computeSlope - Invalid F" << endl;
    throw "NOX Error";
  }

  // Return <v, F> = F' * J * dir = <J'F, dir> = <g, dir>
  return(vecPtr->innerProduct(grp.getF()));
}

double NOX::LineSearch::Utils::Slope::
computeSlopeWithOutJac(const Abstract::Vector& dir, 
		       const Abstract::Group& grp) 
{
  // Allocate space for vecPtr and grpPtr if necessary
  if (Teuchos::is_null(vecPtr)) 
    vecPtr = dir.clone(ShapeCopy);
  if (Teuchos::is_null(grpPtr))
    grpPtr = grp.clone(ShapeCopy);

  // Check that F exists
  if (!grp.isF()) 
  {
    utils.out() << "NOX::LineSearch::Utils::Slope::computeSlope - Invalid F" << endl;
    throw "NOX Error";
  }

  // Compute the perturbation parameter
  double lambda = 1.0e-6;
  double denominator = dir.norm();

  // Don't divide by zero
  if (denominator == 0.0)
    denominator = 1.0;

  double eta = lambda * (lambda + grp.getX().norm() / denominator);

  // Don't divide by zero
  if (eta == 0.0)
    eta = 1.0e-6;

  // Perturb the solution vector
  vecPtr->update(eta, dir, 1.0, grp.getX(), 0.0);

  // Compute the new F --> F(x + eta * dir)
  grpPtr->setX(*vecPtr);  
  grpPtr->computeF();

  // Compute Js = (F(x + eta * dir) - F(x))/eta
  vecPtr->update(-1.0/eta, grp.getF(), 1.0/eta, grpPtr->getF(), 0.0);
  
  return(vecPtr->innerProduct(grp.getF()));
}
