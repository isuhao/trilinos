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

#include "Teuchos_ParameterList.hpp"
#include "NOX_Utils.H"
#include "LOCA_GlobalData.H"
#include "LOCA_ErrorCheck.H"
#include "LOCA_Parameter_Vector.H"
#include "LOCA_TurningPoint_MinimallyAugmented_AbstractGroup.H"
#include "LOCA_TurningPoint_MinimallyAugmented_ExtendedGroup.H"
#include "LOCA_TurningPoint_MinimallyAugmented_Constraint.H"
#include "LOCA_TurningPoint_MinimallyAugmented_ModifiedConstraint.H"
#include "LOCA_MultiContinuation_ConstrainedGroup.H"
#include "LOCA_Parameter_SublistParser.H"
#include "LOCA_Abstract_TransposeSolveGroup.H"

LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
ExtendedGroup(
      const Teuchos::RCP<LOCA::GlobalData>& global_data,
      const Teuchos::RCP<LOCA::Parameter::SublistParser>& topParams,
      const Teuchos::RCP<Teuchos::ParameterList>& tpParams,
      const Teuchos::RCP<LOCA::TurningPoint::MinimallyAugmented::AbstractGroup>& grp)
  : LOCA::Extended::MultiAbstractGroup(),
    LOCA::MultiContinuation::AbstractGroup(),
    globalData(global_data),
    parsedParams(topParams),
    turningPointParams(tpParams),
    grpPtr(grp),
    constraint(),
    conGroup(),
    bifParamID(0)
{
  const char *func = "LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup()";

  // Get bifurcation parameter name
  if (!turningPointParams->isParameter("Bifurcation Parameter")) {
    globalData->locaErrorCheck->throwError(func,
				 "\"Bifurcation Parameter\" name is not set!");
  }
  string bifParamName = turningPointParams->get("Bifurcation Parameter",
						"None");
  const ParameterVector& p = grpPtr->getParams();
  bifParamID = p.getIndex(bifParamName);

  // Create constraint equation
  string constraintMethod = turningPointParams->get("Constraint Method",
						    "Default");
  if (constraintMethod == "Default")
    constraint = 
      Teuchos::rcp(new LOCA::TurningPoint::MinimallyAugmented::Constraint(
							       globalData,
							       parsedParams,
							       tpParams,
							       grpPtr,
							       bifParamID));
  else if (constraintMethod == "Modified")
    constraint = 
      Teuchos::rcp(new LOCA::TurningPoint::MinimallyAugmented::ModifiedConstraint(
							       globalData,
							       parsedParams,
							       tpParams,
							       grpPtr,
							       bifParamID));
  else 
    globalData->locaErrorCheck->throwError(
		    func,
		    string("Unknown constraint method:  ") + constraintMethod);

  // Create constrained group
  std::vector<int> bifParamIDs(1);
  bifParamIDs[0] = bifParamID;
  conGroup = 
    Teuchos::rcp(new LOCA::MultiContinuation::ConstrainedGroup(
							    globalData, 
							    parsedParams,
							    turningPointParams,
							    grpPtr, 
							    constraint,
							    bifParamIDs));
}

LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
ExtendedGroup(
	  const LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup& source,
	  NOX::CopyType type)
  : globalData(source.globalData),
    parsedParams(source.parsedParams),
    turningPointParams(source.turningPointParams),
    grpPtr(),
    constraint(),
    conGroup(),
    bifParamID(source.bifParamID)
{
  conGroup = Teuchos::rcp_dynamic_cast<LOCA::MultiContinuation::ConstrainedGroup>(source.conGroup->clone(type));
  grpPtr = Teuchos::rcp_dynamic_cast<LOCA::TurningPoint::MinimallyAugmented::AbstractGroup>(conGroup->getGroup());
  constraint = Teuchos::rcp_dynamic_cast<LOCA::TurningPoint::MinimallyAugmented::Constraint>(conGroup->getConstraints());
  constraint->setGroup(grpPtr);
}


LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
~ExtendedGroup() 
{
}

NOX::Abstract::Group&
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
operator=(const NOX::Abstract::Group& source)
{
  copy(source);
  return *this;
}

Teuchos::RCP<NOX::Abstract::Group>
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
clone(NOX::CopyType type) const
{
  return Teuchos::rcp(new ExtendedGroup(*this, type));
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
setX(const NOX::Abstract::Vector& y)  
{
  conGroup->setX(y);
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
computeX(const NOX::Abstract::Group& g, 
	 const NOX::Abstract::Vector& d,
	 double step) 
{
  const LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup& mg = 
    dynamic_cast<const LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup&>(g);

  // set newton update in constraint
  Teuchos::RCP<LOCA::TurningPoint::MinimallyAugmented::ModifiedConstraint> mod_constraint = 
    Teuchos::rcp_dynamic_cast<LOCA::TurningPoint::MinimallyAugmented::ModifiedConstraint>(constraint);
  if (mod_constraint != Teuchos::null) {
    const LOCA::MultiContinuation::ExtendedVector& emv_d = 
      dynamic_cast<const LOCA::MultiContinuation::ExtendedVector&>(d);
    mod_constraint->setNewtonUpdates(*(emv_d.getXVec()), emv_d.getScalar(0), 
				     step);
  }

  conGroup->computeX(*(mg.conGroup), d, step);
}

NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
computeF() 
{
  return conGroup->computeF();
}

NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
computeJacobian() 
{
  return conGroup->computeJacobian();
}

NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
computeGradient() 
{
  return conGroup->computeGradient();
}
   
NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
computeNewton(Teuchos::ParameterList& params) 
{
  return conGroup->computeNewton(params);
}

NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
applyJacobian(const NOX::Abstract::Vector& input,
	      NOX::Abstract::Vector& result) const 
{
  return conGroup->applyJacobian(input, result);
}

NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
applyJacobianTranspose(const NOX::Abstract::Vector& input,
		       NOX::Abstract::Vector& result) const 
{
  return conGroup->applyJacobianTranspose(input, result);
}

NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
applyJacobianInverse(Teuchos::ParameterList& params, 
		     const NOX::Abstract::Vector& input,
		     NOX::Abstract::Vector& result) const 
{
  return conGroup->applyJacobianInverse(params, input, result);
}

NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
applyJacobianMultiVector(const NOX::Abstract::MultiVector& input,
			 NOX::Abstract::MultiVector& result) const 
{
  return conGroup->applyJacobianMultiVector(input, result);
}

NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
applyJacobianTransposeMultiVector(const NOX::Abstract::MultiVector& input,
				  NOX::Abstract::MultiVector& result) const 
{
  return conGroup->applyJacobianTransposeMultiVector(input, result);
}

NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
applyJacobianInverseMultiVector(Teuchos::ParameterList& params,
				const NOX::Abstract::MultiVector& input,
				NOX::Abstract::MultiVector& result) const 
{
  return conGroup->applyJacobianInverseMultiVector(params, input, result);
}

bool
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
isF() const 
{
  return conGroup->isF();
}

bool
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
isJacobian() const 
{
  return conGroup->isJacobian();
}

bool
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
isGradient() const 
{
  return conGroup->isGradient();
}

bool
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
isNewton() const 
{
  return conGroup->isNewton();
}
  
const NOX::Abstract::Vector&
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getX() const 
{
  return conGroup->getX();
}

const NOX::Abstract::Vector&
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getF() const 
{
  return conGroup->getF();
}

double
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getNormF() const 
{
  return conGroup->getNormF();
}

const NOX::Abstract::Vector&
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getGradient() const 
{
  return conGroup->getGradient();
}

const NOX::Abstract::Vector&
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getNewton() const 
{
  return conGroup->getNewton();
}

double
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getNormNewtonSolveResidual() const 
{
  return conGroup->getNormNewtonSolveResidual();
}

Teuchos::RCP<const LOCA::MultiContinuation::AbstractGroup>
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getUnderlyingGroup() const
{
  return conGroup->getUnderlyingGroup();
}

Teuchos::RCP<LOCA::MultiContinuation::AbstractGroup>
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getUnderlyingGroup()
{
  return conGroup->getUnderlyingGroup();
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
copy(const NOX::Abstract::Group& src) 
{

  const LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup& source = 
    dynamic_cast<const LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup&>(src);

  // Protect against A = A
  if (this != &source) {
    globalData = source.globalData;
    parsedParams = source.parsedParams;
    turningPointParams = source.turningPointParams;
    conGroup->copy(*source.conGroup);
    grpPtr = Teuchos::rcp_dynamic_cast<LOCA::TurningPoint::MinimallyAugmented::AbstractGroup>(conGroup->getGroup());
    constraint = Teuchos::rcp_dynamic_cast<LOCA::TurningPoint::MinimallyAugmented::Constraint>(conGroup->getConstraints());
    constraint->setGroup(grpPtr);
    bifParamID = source.bifParamID;
  }
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
setParamsMulti(const vector<int>& paramIDs, 
	       const NOX::Abstract::MultiVector::DenseMatrix& vals)
{
  conGroup->setParamsMulti(paramIDs, vals);
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
setParams(const LOCA::ParameterVector& p)
{
  conGroup->setParams(p);
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
setParam(int paramID, double val)
{
  conGroup->setParam(paramID, val);
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
setParam(string paramID, double val)
{
  conGroup->setParam(paramID, val);
}

const LOCA::ParameterVector&
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getParams() const
{
  return conGroup->getParams();
}

double
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getParam(int paramID) const
{
  return conGroup->getParam(paramID);
}

double
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getParam(string paramID) const
{
  return conGroup->getParam(paramID);
}

NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
computeDfDpMulti(const vector<int>& paramIDs, 
		 NOX::Abstract::MultiVector& dfdp, 
		 bool isValidF)
{
  return conGroup->computeDfDpMulti(paramIDs, dfdp, isValidF);
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
preProcessContinuationStep(LOCA::Abstract::Iterator::StepStatus stepStatus)
{
  conGroup->preProcessContinuationStep(stepStatus);
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
postProcessContinuationStep(LOCA::Abstract::Iterator::StepStatus stepStatus)
{
  conGroup->postProcessContinuationStep(stepStatus);
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
projectToDraw(const NOX::Abstract::Vector& x,
	      double *px) const
{
  conGroup->projectToDraw(x, px);
}

int
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
projectToDrawDimension() const
{
  return conGroup->projectToDrawDimension();
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
printSolution(const double conParam) const
{
  if (globalData->locaUtils->isPrintType(NOX::Utils::StepperDetails)) {
    globalData->locaUtils->out() << 
      "LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::printSolution\n";

    globalData->locaUtils->out() << "Turning Point located at: " << 
      globalData->locaUtils->sciformat(conParam) << "   " << 
      globalData->locaUtils->sciformat(getBifParam()) << std::endl;

    globalData->locaUtils->out() << 
      "\tPrinting Solution Vector for conParam = " << 
      globalData->locaUtils->sciformat(conParam) << std::endl;
  }
  grpPtr->printSolution(conParam);
  if (globalData->locaUtils->isPrintType(NOX::Utils::StepperDetails)) {
    globalData->locaUtils->out() << 
      "\tPrinting Right Null Vector for bif param = " << 
      globalData->locaUtils->sciformat(getBifParam()) << std::endl;
  }
  grpPtr->printSolution(*(constraint->getRightNullVec()), getBifParam());
  if (globalData->locaUtils->isPrintType(NOX::Utils::StepperDetails)) {
    globalData->locaUtils->out() << 
      "\tPrinting Left Null Vector for sigma = " << 
      globalData->locaUtils->sciformat(constraint->getSigma()) << std::endl;
  }
  grpPtr->printSolution(*(constraint->getLeftNullVec()), 
			constraint->getSigma());
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
printSolution(const NOX::Abstract::Vector& x,
	      const double conParam) const
{
  const LOCA::MultiContinuation::ExtendedVector& tp_x = 
    dynamic_cast<const LOCA::MultiContinuation::ExtendedVector&>(x);

  if (globalData->locaUtils->isPrintType(NOX::Utils::StepperDetails)) {
    globalData->locaUtils->out() << 
      "LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::printSolution\n";

    globalData->locaUtils->out() << "Turning Point located at: " << 
      globalData->locaUtils->sciformat(conParam) << "   " << 
      globalData->locaUtils->sciformat(tp_x.getScalar(0)) << std::endl;

    globalData->locaUtils->out() << 
      "\tPrinting Solution Vector for conParam = " << 
      globalData->locaUtils->sciformat(conParam) << std::endl;
  }
  grpPtr->printSolution(*(tp_x.getXVec()), conParam);
  if (globalData->locaUtils->isPrintType(NOX::Utils::StepperDetails)) {
    globalData->locaUtils->out() << 
      "\tPrinting Right Null Vector for bif param = " << 
      globalData->locaUtils->sciformat(tp_x.getScalar(0)) << std::endl;
  }
  grpPtr->printSolution(*(constraint->getRightNullVec()), tp_x.getScalar(0));
  if (globalData->locaUtils->isPrintType(NOX::Utils::StepperDetails)) {
    globalData->locaUtils->out() << 
      "\tPrinting Left Null Vector for sigma = " << 
      globalData->locaUtils->sciformat(constraint->getSigma()) << std::endl;
  }
  grpPtr->printSolution(*(constraint->getLeftNullVec()), 
			constraint->getSigma());
}

int
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getBorderedWidth() const
{
  return conGroup->getBorderedWidth();
}

Teuchos::RCP<const NOX::Abstract::Group>
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getUnborderedGroup() const
{
  return conGroup->getUnborderedGroup();
}

bool
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
isCombinedAZero() const
{
  return conGroup->isCombinedAZero();
}

bool
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
isCombinedBZero() const
{
  return conGroup->isCombinedBZero();
}

bool
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
isCombinedCZero() const
{
  return conGroup->isCombinedCZero();
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
extractSolutionComponent(const NOX::Abstract::MultiVector& v,
			 NOX::Abstract::MultiVector& v_x) const
{
  conGroup->extractSolutionComponent(v, v_x);
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
extractParameterComponent(bool use_transpose,
			  const NOX::Abstract::MultiVector& v,
			  NOX::Abstract::MultiVector::DenseMatrix& v_p) const
{
  conGroup->extractParameterComponent(use_transpose, v, v_p);
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
loadNestedComponents(const NOX::Abstract::MultiVector& v_x,
		     const NOX::Abstract::MultiVector::DenseMatrix& v_p,
		     NOX::Abstract::MultiVector& v) const
{
  conGroup->loadNestedComponents(v_x, v_p, v);
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
fillA(NOX::Abstract::MultiVector& A) const
{
  conGroup->fillA(A);
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
fillB(NOX::Abstract::MultiVector& B) const
{
  conGroup->fillB(B);
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
fillC(NOX::Abstract::MultiVector::DenseMatrix& C) const
{
  conGroup->fillC(C);
}

NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
applyJacobianTransposeInverse(Teuchos::ParameterList& params, 
		     const NOX::Abstract::Vector& input,
		     NOX::Abstract::Vector& result) const 
{
  return conGroup->applyJacobianTransposeInverse(params, input, result);
}

NOX::Abstract::Group::ReturnType
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
applyJacobianTransposeInverseMultiVector(Teuchos::ParameterList& params,
					 const NOX::Abstract::MultiVector& input,
					 NOX::Abstract::MultiVector& result) const 
{
  return conGroup->applyJacobianTransposeInverseMultiVector(params, input, result);
}

double
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getBifParam() const
{
  return grpPtr->getParam(bifParamID);
}

Teuchos::RCP<const NOX::Abstract::Vector>
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getLeftNullVec() const
{
  return constraint->getLeftNullVec();
}

Teuchos::RCP<const NOX::Abstract::Vector>
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getRightNullVec() const
{
  return constraint->getRightNullVec();
}

Teuchos::RCP<const NOX::Abstract::Vector>
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getAVec() const
{
  return constraint->getAVec();
}

Teuchos::RCP<const NOX::Abstract::Vector>
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
getBVec() const
{
  return constraint->getBVec();
}

void
LOCA::TurningPoint::MinimallyAugmented::ExtendedGroup::
setBifParam(double param)
{
  conGroup->setParam(bifParamID, param);
}
