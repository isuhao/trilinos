// @HEADER
// ***********************************************************************
//
//                 Anasazi: Block Eigensolvers Package
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
// ***********************************************************************
// @HEADER

#ifndef ANASAZI_LOBPCG_SOLMGR_HPP
#define ANASAZI_LOBPCG_SOLMGR_HPP

/*! \file AnasaziLOBPCGSolMgr.hpp
 *  \brief The Anasazi::LOBPCGSolMgr provides a powerful solver manager for the LOBPCG eigensolver.
*/

#include "AnasaziConfigDefs.hpp"
#include "AnasaziTypes.hpp"

#include "AnasaziEigenproblem.hpp"
#include "AnasaziSolverManager.hpp"
#include "AnasaziSolverUtils.hpp"

#include "AnasaziLOBPCG.hpp"
#include "AnasaziBasicSort.hpp"
#include "AnasaziSVQBOrthoManager.hpp"
#include "AnasaziBasicOrthoManager.hpp"
#include "AnasaziStatusTestMaxIters.hpp"
#include "AnasaziStatusTestResNorm.hpp"
#include "AnasaziStatusTestWithOrdering.hpp"
#include "AnasaziStatusTestCombo.hpp"
#include "AnasaziStatusTestOutput.hpp"
#include "AnasaziBasicOutputManager.hpp"
#include "Teuchos_BLAS.hpp"
#include "Teuchos_TimeMonitor.hpp"


/** \example LOBPCG/LOBPCGEpetraEx.cpp
    This is an example of how to use the Anasazi::LOBPCGSolMgr solver manager to solve a standard eigenvalue problem, using Epetra data structures.
*/

/** \example LOBPCG/LOBPCGEpetraExGen.cpp
    This is an example of how to use the Anasazi::LOBPCGSolMgr solver manager to solve a generalized eigenvalue problem, using Epetra data stuctures.
*/

/** \example LOBPCG/LOBPCGEpetraExGenPrecIfpack.cpp
    This is an example of how to use the Anasazi::LOBPCGSolMgr solver manager to solve a generalized eigenvalue problem, using Epetra data structures and exploiting a incomplete Cholesky preconditioner from IFPACK.
*/


namespace Anasazi {


/*! \class LOBPCGSolMgr
 *
 *  \brief The LOBPCGSolMgr provides a powerful solver manager over the LOBPCG eigensolver.
 *
 * This solver manager exists to provide a flexible manager over the LOBPCG eigensolver intended for general use. Features 
 * provided by this solver manager include:
 *   - locking of converged eigenpairs
 *   - global convergence on only the significant eigenpairs (instead of any eigenpairs with low residual)
 *   - recovery from LOBPCGRitzFailure when full orthogonalization is disabled
 *
 * The solver manager provides to the solver a StatusTestCombo object constructed as follows:<br>
 *    &nbsp;&nbsp;&nbsp;<tt>combo = maxiterstest OR globaltest OR lockingtest OR debugtest</tt><br>
 * where
 *    - \c maxiters terminates computation when a maximum number of iterations have been performed<br>
 *      \c maxiters is a StatusTestMaxIters object
 *    - \c globaltest terminates computation when global convergence has been detected.<br>
 *      It is encapsulated in a StatusTestWithOrdering object, to ensure that computation is terminated
 *      only after the most significant eigenvalues/eigenvectors have met the convergence criteria.<br>
 *      If not specified via setGlobalStatusTest(), \c globaltest is a StatusTestResNorm object which tests the
 *      M-norms of the direct residuals relative to the Ritz values.
 *    - \c lockingtest halts LOBPCG::iterate() in order to deflate converged eigenpairs for locking.<br>
 *      It will query the underlying LOBPCG eigensolver to determine when eigenvectors should be locked.<br>
 *      If not specified via setLockingStatusTest(), \c lockingtest is a StatusTestResNorm object.
 *    - \c debugtest allows a user to specify additional monitoring of the iteration, encapsulated in a StatusTest object<br>
 *      If not specified via setDebugStatusTest(), \c debugtest is ignored.<br> 
 *      In most cases, it should return ::Failed; if it returns ::Passed, solve() will throw an AnasaziError exception.
 *
 * Much of this behavior is controlled via parameters and options passed to the
 * solver manager. For more information, see LOBPCGSolMgr().

 \ingroup anasazi_solver_framework

 \author Chris Baker, Ulrich Hetmaniuk, Rich Lehoucq, Heidi Thornquist
 */

template<class ScalarType, class MV, class OP>
class LOBPCGSolMgr : public SolverManager<ScalarType,MV,OP> {

  private:
    typedef MultiVecTraits<ScalarType,MV> MVT;
    typedef OperatorTraits<ScalarType,MV,OP> OPT;
    typedef Teuchos::ScalarTraits<ScalarType> SCT;
    typedef typename Teuchos::ScalarTraits<ScalarType>::magnitudeType MagnitudeType;
    typedef Teuchos::ScalarTraits<MagnitudeType> MT;
    
  public:

  //! @name Constructors/Destructor
  //@{ 

  /*! \brief Basic constructor for LOBPCGSolMgr.
   *
   * This constructor accepts the Eigenproblem to be solved in addition
   * to a parameter list of options for the solver manager. These options include the following:
   *   - Solver parameters
   *      - \c "Which" - a \c string specifying the desired eigenvalues: SM, LM, SR or LR. Default: "SR"
   *      - \c "Block Size" - a \c int specifying the block size to be used by the underlying LOBPCG solver. Default: problem->getNEV()
   *      - \c "Full Ortho" - a \c bool specifying whether the underlying solver should employ the full orthogonalization scheme. Default: true
   *      - \c "Recover" - a \c bool specifying whether the solver manager should attempt to recover in the case of a LOBPCGRitzFailure when full orthogonalization is disabled. Default: true
   *      - \c "Verbosity" - a sum of MsgType specifying the verbosity. Default: ::Errors
   *      - \c "Init" - a LOBPCGState<ScalarType,MV> struct used to initialize the LOBPCG eigensolver.
   *   - Convergence parameters (if using default convergence test; see setGlobalStatusTest())
   *      - \c "Maximum Iterations" - a \c int specifying the maximum number of iterations the underlying solver is allowed to perform. Default: 100
   *      - \c "Convergence Tolerance" - a \c MagnitudeType specifying the level that residual norms must reach to decide convergence. Default: machine precision.
   *      - \c "Relative Convergence Tolerance" - a \c bool specifying whether residuals norms should be scaled by their eigenvalues for the purposing of deciding convergence. Default: true
   *      - \c "Convergence Norm" - a \c string specifying the norm for convergence testing: "2" or "M" 
   *   - Locking parameters (if using default locking test; see setLockingStatusTest())
   *      - \c "Use Locking" - a \c bool specifying whether the algorithm should employ locking of converged eigenpairs. Default: false
   *      - \c "Max Locked" - a \c int specifying the maximum number of eigenpairs to be locked. Default: problem->getNEV()
   *      - \c "Locking Quorum" - a \c int specifying the number of eigenpairs that must meet the locking criteria before locking actually occurs. Default: 1
   *      - \c "Locking Tolerance" - a \c MagnitudeType specifying the level that residual norms must reach to decide locking. Default: 0.1*convergence tolerance
   *      - \c "Relative Locking Tolerance" - a \c bool specifying whether residuals norms should be scaled by their eigenvalues for the purposing of deciding locking. Default: true
   *      - \c "Locking Norm" - a \c string specifying the norm for locking testing: "2" or "M" 
   */
  LOBPCGSolMgr( const Teuchos::RCP<Eigenproblem<ScalarType,MV,OP> > &problem,
                             Teuchos::ParameterList &pl );

  //! Destructor.
  virtual ~LOBPCGSolMgr() {};
  //@}
  
  //! @name Accessor methods
  //@{ 

  //! Return the eigenvalue problem.
  const Eigenproblem<ScalarType,MV,OP>& getProblem() const {
    return *problem_;
  }

  //! Get the iteration count for the most recent call to \c solve().
  int getNumIters() const { 
    return numIters_; 
  }

  /*! \brief Return the timers for this object. 
   *
   * The timers are ordered as follows:
   *   - time spent in solve() routine
   *   - time spent locking converged eigenvectors
   */
   Teuchos::Array<Teuchos::RCP<Teuchos::Time> > getTimers() const {
     return tuple(_timerSolve, _timerLocking);
   }


  //@}

  //! @name Solver application methods
  //@{ 
    
  /*! \brief This method performs possibly repeated calls to the underlying eigensolver's iterate() routine
   * until the problem has been solved (as decided by the solver manager) or the solver manager decides to 
   * quit.
   *
   * This method calls LOBPCG::iterate(), which will return either because a specially constructed status test evaluates to ::Passed
   * or an exception is thrown.
   *
   * A return from LOBPCG::iterate() signifies one of the following scenarios:
   *    - the maximum number of iterations has been exceeded. In this scenario, the solver manager will place\n
   *      all converged eigenpairs into the eigenproblem and return ::Unconverged.
   *    - the locking conditions have been met. In this scenario, some of the current eigenpairs will be removed\n
   *      from the eigensolver and placed into auxiliary storage. The eigensolver will be restarted with the remaining\n
   *      eigenpairs and some random information to replace the removed eigenpairs.
   *    - global convergence has been met. In this case, the most significant NEV eigenpairs in the solver and locked storage  \n
   *      have met the convergence criterion. (Here, NEV refers to the number of eigenpairs requested by the Eigenproblem.)    \n
   *      In this scenario, the solver manager will return ::Converged.
   *    - an LOBPCGRitzFailure exception has been thrown. If full orthogonalization is enabled and recovery from this exception\n
   *      is requested, the solver manager will attempt to recover from this exception by gathering the current eigenvectors,  \n
   *      preconditioned residual, and search directions from the eigensolver, orthogonormalizing the basis composed of these  \n
   *      three, projecting the eigenproblem, and restarting the eigensolver with the solution of the project eigenproblem. Any \n
   *      additional failure that occurs during this recovery effort will result in the eigensolver returning ::Unconverged.
   *
   * \returns ::ReturnType specifying:
   *    - ::Converged: the eigenproblem was solved to the specification required by the solver manager.
   *    - ::Unconverged: the eigenproblem was not solved to the specification desired by the solver manager
  */
  ReturnType solve();

  //! Set the status test defining global convergence.
  void setGlobalStatusTest(const Teuchos::RCP< StatusTest<ScalarType,MV,OP> > &global);

  //! Get the status test defining global convergence.
  const Teuchos::RCP< StatusTest<ScalarType,MV,OP> > & getGlobalStatusTest() const;

  //! Set the status test defining locking.
  void setLockingStatusTest(const Teuchos::RCP< StatusTest<ScalarType,MV,OP> > &locking);

  //! Get the status test defining locking.
  const Teuchos::RCP< StatusTest<ScalarType,MV,OP> > & getLockingStatusTest() const;

  //! Set the status test for debugging.
  void setDebugStatusTest(const Teuchos::RCP< StatusTest<ScalarType,MV,OP> > &debug);

  //! Get the status test for debugging.
  const Teuchos::RCP< StatusTest<ScalarType,MV,OP> > & getDebugStatusTest() const;

  //@}

  private:
  Teuchos::RCP<Eigenproblem<ScalarType,MV,OP> > problem_;

  std::string whch_, ortho_;

  MagnitudeType convtol_, locktol_;
  int maxIters_, numIters_;
  bool useLocking_;
  bool relconvtol_, rellocktol_;
  int blockSize_;
  bool fullOrtho_;
  int maxLocked_;
  int verbosity_;
  int lockQuorum_;
  bool recover_;
  Teuchos::RCP<LOBPCGState<ScalarType,MV> > state_;
  enum StatusTestResNorm<ScalarType,MV,OP>::ResType convNorm_, lockNorm_;

  Teuchos::RCP<Teuchos::Time> _timerSolve, _timerLocking;

  Teuchos::RCP<StatusTest<ScalarType,MV,OP> > globalTest_;
  Teuchos::RCP<StatusTest<ScalarType,MV,OP> > lockingTest_; 
  Teuchos::RCP<StatusTest<ScalarType,MV,OP> > debugTest_;
};


// Constructor
template<class ScalarType, class MV, class OP>
LOBPCGSolMgr<ScalarType,MV,OP>::LOBPCGSolMgr( 
        const Teuchos::RCP<Eigenproblem<ScalarType,MV,OP> > &problem,
        Teuchos::ParameterList &pl ) : 
  problem_(problem),
  whch_("SR"),
  ortho_("SVQB"),
  convtol_(MT::prec()),
  maxIters_(100),
  numIters_(0),
  useLocking_(false),
  relconvtol_(true),
  rellocktol_(true),
  blockSize_(0),
  fullOrtho_(true),
  maxLocked_(0),
  verbosity_(Anasazi::Errors),
  lockQuorum_(1),
  recover_(true),
  _timerSolve(Teuchos::TimeMonitor::getNewTimer("LOBPCGSolMgr::solve()")),
  _timerLocking(Teuchos::TimeMonitor::getNewTimer("LOBPCGSolMgr locking"))
{
  TEST_FOR_EXCEPTION(problem_ == Teuchos::null,              std::invalid_argument, "Problem not given to solver manager.");
  TEST_FOR_EXCEPTION(!problem_->isProblemSet(),              std::invalid_argument, "Problem not set.");
  TEST_FOR_EXCEPTION(!problem_->isHermitian(),               std::invalid_argument, "Problem not symmetric.");
  TEST_FOR_EXCEPTION(problem_->getInitVec() == Teuchos::null,std::invalid_argument, "Problem does not contain initial vectors to clone from.");


  std::string strtmp;

  // which values to solve for
  whch_ = pl.get("Which",whch_);
  TEST_FOR_EXCEPTION(whch_ != "SM" && whch_ != "LM" && whch_ != "SR" && whch_ != "LR",
      std::invalid_argument, "Anasazi::LOBPCGSolMgr: Invalid sorting string.");

  // which orthogonalization to use
  ortho_ = pl.get("Orthogonalization",ortho_);
  if (ortho_ != "DGKS" && ortho_ != "SVQB") {
    ortho_ = "SVQB";
  }

  // convergence tolerance
  convtol_ = pl.get("Convergence Tolerance",convtol_);
  relconvtol_ = pl.get("Relative Convergence Tolerance",relconvtol_);
  strtmp = pl.get("Convergence Norm",std::string("2"));
  if (strtmp == "2") {
    convNorm_ = StatusTestResNorm<ScalarType,MV,OP>::RES_2NORM;
  }
  else if (strtmp == "M") {
    convNorm_ = StatusTestResNorm<ScalarType,MV,OP>::RES_ORTH;
  }
  else {
    TEST_FOR_EXCEPTION(true, std::invalid_argument, 
        "Anasazi::LOBPCGSolMgr: Invalid Convergence Norm.");
  }

  
  // locking tolerance
  useLocking_ = pl.get("Use Locking",useLocking_);
  rellocktol_ = pl.get("Relative Locking Tolerance",rellocktol_);
  // default: should be less than convtol_
  locktol_ = convtol_/10;
  locktol_ = pl.get("Locking Tolerance",locktol_);
  strtmp = pl.get("Locking Norm",std::string("2"));
  if (strtmp == "2") {
    lockNorm_ = StatusTestResNorm<ScalarType,MV,OP>::RES_2NORM;
  }
  else if (strtmp == "M") {
    lockNorm_ = StatusTestResNorm<ScalarType,MV,OP>::RES_ORTH;
  }
  else {
    TEST_FOR_EXCEPTION(true, std::invalid_argument, 
        "Anasazi::LOBPCGSolMgr: Invalid Locking Norm.");
  }

  // maximum number of iterations
  maxIters_ = pl.get("Maximum Iterations",maxIters_);

  // block size: default is nev()
  blockSize_ = pl.get("Block Size",problem_->getNEV());
  TEST_FOR_EXCEPTION(blockSize_ <= 0, std::invalid_argument,
                     "Anasazi::LOBPCGSolMgr: \"Block Size\" must be strictly positive.");

  // max locked: default is nev(), must satisfy maxLocked_ + blockSize_ >= nev
  if (useLocking_) {
    maxLocked_ = pl.get("Max Locked",problem_->getNEV());
  }
  else {
    maxLocked_ = 0;
  }
  if (maxLocked_ == 0) {
    useLocking_ = false;
  }
  TEST_FOR_EXCEPTION(maxLocked_ < 0, std::invalid_argument,
                     "Anasazi::LOBPCGSolMgr: \"Max Locked\" must be positive.");
  TEST_FOR_EXCEPTION(maxLocked_ + blockSize_ < problem_->getNEV(), 
                     std::invalid_argument,
                     "Anasazi::LOBPCGSolMgr: Not enough storage space for requested number of eigenpairs.");

  if (useLocking_) {
    lockQuorum_ = pl.get("Locking Quorum",lockQuorum_);
    TEST_FOR_EXCEPTION(lockQuorum_ <= 0,
                       std::invalid_argument,
                       "Anasazi::LOBPCGSolMgr: \"Locking Quorum\" must be strictly positive.");
  }

  // full orthogonalization: default true
  fullOrtho_ = pl.get("Full Ortho",fullOrtho_);

  // verbosity level
  if (pl.isParameter("Verbosity")) {
    if (Teuchos::isParameterType<int>(pl,"Verbosity")) {
      verbosity_ = pl.get("Verbosity", verbosity_);
    } else {
      verbosity_ = (int)Teuchos::getParameter<Anasazi::MsgType>(pl,"Verbosity");
    }
  }

  // recover from LOBPCGRitzFailure
  recover_ = pl.get("Recover",recover_);

  // get (optionally) an initial state
  if (pl.isParameter("Init")) {
    state_ = Teuchos::getParameter<Teuchos::RCP<Anasazi::LOBPCGState<ScalarType,MV> > >(pl,"Init");
  }
}


// solve()
template<class ScalarType, class MV, class OP>
ReturnType 
LOBPCGSolMgr<ScalarType,MV,OP>::solve() {

  typedef SolverUtils<ScalarType,MV,OP> msutils;

  const int nev = problem_->getNEV();



  //////////////////////////////////////////////////////////////////////////////////////
  // Sort manager
  Teuchos::RCP<BasicSort<MagnitudeType> > sorter = Teuchos::rcp( new BasicSort<MagnitudeType>(whch_) );

  //////////////////////////////////////////////////////////////////////////////////////
  // Output manager
  Teuchos::RCP<BasicOutputManager<ScalarType> > printer = Teuchos::rcp( new BasicOutputManager<ScalarType>(verbosity_) );

  //////////////////////////////////////////////////////////////////////////////////////
  // Status tests
  //
  // maximum number of iterations: optional test
  Teuchos::RCP<StatusTestMaxIters<ScalarType,MV,OP> > maxtest;
  if (maxIters_ > 0) {
    maxtest = Teuchos::rcp( new StatusTestMaxIters<ScalarType,MV,OP>(maxIters_) );
  }
  // convergence
  Teuchos::RCP<StatusTest<ScalarType,MV,OP> > convtest;
  if (globalTest_ == Teuchos::null) {
    convtest = Teuchos::rcp( new StatusTestResNorm<ScalarType,MV,OP>(convtol_,nev,convNorm_,relconvtol_) );
  }
  else {
    convtest = globalTest_;
  }
  Teuchos::RCP<StatusTestWithOrdering<ScalarType,MV,OP> > ordertest 
    = Teuchos::rcp( new StatusTestWithOrdering<ScalarType,MV,OP>(convtest,sorter,nev) );
  // locking
  Teuchos::RCP<StatusTest<ScalarType,MV,OP> > locktest;
  if (useLocking_) {
    if (lockingTest_ == Teuchos::null) {
      locktest = Teuchos::rcp( new StatusTestResNorm<ScalarType,MV,OP>(locktol_,lockQuorum_,lockNorm_,rellocktol_) );
    }
    else {
      locktest = lockingTest_;
    }
  }
  // for a non-short-circuited OR test, the order doesn't matter
  Teuchos::Array<Teuchos::RCP<StatusTest<ScalarType,MV,OP> > > alltests;
  alltests.push_back(ordertest);
  if (locktest != Teuchos::null) alltests.push_back(locktest);
  if (debugTest_ != Teuchos::null) alltests.push_back(debugTest_);
  if (maxtest != Teuchos::null) alltests.push_back(maxtest);
  Teuchos::RCP<StatusTestCombo<ScalarType,MV,OP> > combotest
    = Teuchos::rcp( new StatusTestCombo<ScalarType,MV,OP>( StatusTestCombo<ScalarType,MV,OP>::OR, alltests) );
  // printing StatusTest
  Teuchos::RCP<StatusTestOutput<ScalarType,MV,OP> > outputtest;
  if ( printer->isVerbosity(Debug) ) {
    outputtest = Teuchos::rcp( new StatusTestOutput<ScalarType,MV,OP>( printer,combotest,1,Passed+Failed+Undefined ) );
  }
  else {
    outputtest = Teuchos::rcp( new StatusTestOutput<ScalarType,MV,OP>( printer,combotest,1,Passed ) );
  }

  //////////////////////////////////////////////////////////////////////////////////////
  // Orthomanager
  Teuchos::RCP<MatOrthoManager<ScalarType,MV,OP> > ortho; 
  if (ortho_=="SVQB") {
    ortho = Teuchos::rcp( new SVQBOrthoManager<ScalarType,MV,OP>(problem_->getM()) );
  } else if (ortho_=="DGKS") {
    ortho = Teuchos::rcp( new BasicOrthoManager<ScalarType,MV,OP>(problem_->getM()) );
  } else {
    TEST_FOR_EXCEPTION(ortho_!="SVQB"&&ortho_!="DGKS",std::logic_error,"Anasazi::LOBPCGSolMgr::solve(): Invalid orthogonalization type.");
  }

  //////////////////////////////////////////////////////////////////////////////////////
  // Parameter list
  Teuchos::ParameterList plist;
  plist.set("Block Size",blockSize_);
  plist.set("Full Ortho",fullOrtho_);

  //////////////////////////////////////////////////////////////////////////////////////
  // LOBPCG solver
  Teuchos::RCP<LOBPCG<ScalarType,MV,OP> > lobpcg_solver 
    = Teuchos::rcp( new LOBPCG<ScalarType,MV,OP>(problem_,sorter,printer,outputtest,ortho,plist) );
  // set any auxiliary vectors defined in the problem
  Teuchos::RCP< const MV > probauxvecs = problem_->getAuxVecs();
  if (probauxvecs != Teuchos::null) {
    lobpcg_solver->setAuxVecs( Teuchos::tuple< Teuchos::RCP<const MV> >(probauxvecs) );
  }

  //////////////////////////////////////////////////////////////////////////////////////
  // Storage
  // 
  // lockvecs will contain eigenvectors that have been determined "locked" by the status test
  int curNumLocked = 0;
  Teuchos::RCP<MV> lockvecs;
  if (useLocking_) {
    lockvecs = MVT::Clone(*problem_->getInitVec(),maxLocked_);
  }
  std::vector<MagnitudeType> lockvals;
  // workMV will be used as work space for LOBPCGRitzFailure recovery and locking
  // it will be partitioned in these cases as follows:
  // for LOBPCGRitzFailure recovery:
  // workMV = [X H P OpX OpH OpP], where OpX OpH OpP will be used for K and M
  // total size: 2*3*blocksize
  // for locking
  // workMV = [X P MX MP], with MX,MP needing storage only if hasM==true
  // total size: 2*blocksize or 4*blocksize
  Teuchos::RCP<MV> workMV;
  if (fullOrtho_ == false && recover_ == true) {
    workMV = MVT::Clone(*problem_->getInitVec(),2*3*blockSize_);
  }
  else if (useLocking_) {
    if (problem_->getM() != Teuchos::null) {
      workMV = MVT::Clone(*problem_->getInitVec(),4*blockSize_);
    }
    else {
      workMV = MVT::Clone(*problem_->getInitVec(),2*blockSize_);
    }
  }

  // initialize the solution to nothing in case we throw an exception
  Eigensolution<ScalarType,MV> sol;
  sol.numVecs = 0;
  problem_->setSolution(sol);

  // initialize the solver if the user specified a state
  if (state_ != Teuchos::null) {
    lobpcg_solver->initialize(*state_);
  }

  // enter solve() iterations
  {
    Teuchos::TimeMonitor slvtimer(*_timerSolve);

    // tell the lobpcg_solver to iterate
    while (1) {
      try {
        lobpcg_solver->iterate();

        ////////////////////////////////////////////////////////////////////////////////////
        //
        // check user-specified debug test; if it passed, return an exception
        //
        ////////////////////////////////////////////////////////////////////////////////////
        if (debugTest_ != Teuchos::null && debugTest_->getStatus() == Passed) {
          throw AnasaziError("Anasazi::LOBPCGSolMgr::solve(): User-specified debug status test returned Passed.");
        }
        ////////////////////////////////////////////////////////////////////////////////////
        //
        // check convergence first
        //
        ////////////////////////////////////////////////////////////////////////////////////
        else if (ordertest->getStatus() == Passed || (maxtest != Teuchos::null && maxtest->getStatus() == Passed) ) {
          // we have convergence or not
          // ordertest->whichVecs() tells us which vectors from lockvecs and solver state are the ones we want
          // ordertest->howMany() will tell us how many
          break;
        }
        ////////////////////////////////////////////////////////////////////////////////////
        //
        // check locking if we didn't converge
        //
        ////////////////////////////////////////////////////////////////////////////////////
        else if (locktest != Teuchos::null && locktest->getStatus() == Passed) {

          Teuchos::TimeMonitor lcktimer(*_timerLocking);

          // remove the locked vectors,values from lobpcg_solver: put them in newvecs, newvals
          TEST_FOR_EXCEPTION(locktest->howMany() <= 0,std::logic_error,
              "Anasazi::LOBPCGSolMgr::solve(): status test mistake: howMany() non-positive.");
          TEST_FOR_EXCEPTION(locktest->howMany() != (int)locktest->whichVecs().size(),std::logic_error,
              "Anasazi::LOBPCGSolMgr::solve(): status test mistake: howMany() not consistent with whichVecs().");
          TEST_FOR_EXCEPTION(curNumLocked == maxLocked_,std::logic_error,
              "Anasazi::LOBPCGSolMgr::solve(): status test mistake: locking not deactivated.");
          // get the indices
          int numnew = locktest->howMany();
          std::vector<int> indnew = locktest->whichVecs();

          // don't lock more than maxLocked_; we didn't allocate enough space.
          if (curNumLocked + numnew > maxLocked_) {
            numnew = maxLocked_ - curNumLocked;
            indnew.resize(numnew);
          }

          // the call below to lobpcg_solver->setAuxVecs() will reset the solver to unitialized with hasP() == false
          // store the hasP() state for use below
          bool hadP = lobpcg_solver->hasP();

          {
            // debug printing
            printer->print(Debug,"Locking vectors: ");
            for (unsigned int i=0; i<indnew.size(); i++) {printer->stream(Debug) << " " << indnew[i];}
            printer->print(Debug,"\n");
          }
          std::vector<MagnitudeType> newvals(numnew);
          Teuchos::RCP<const MV> newvecs;
          {
            // work in a local scope, to hide the variabes needed for extracting this info
            // get the vectors
            newvecs = MVT::CloneView(*lobpcg_solver->getRitzVectors(),indnew);
            // get the values
            std::vector<Value<ScalarType> > allvals = lobpcg_solver->getRitzValues();
            for (int i=0; i<numnew; i++) {
              newvals[i] = allvals[indnew[i]].realpart;
            }
          }
          // put newvecs into lockvecs
          {
            std::vector<int> indlock(numnew);
            for (int i=0; i<numnew; i++) indlock[i] = curNumLocked+i;
            MVT::SetBlock(*newvecs,indlock,*lockvecs);
            newvecs = Teuchos::null;
          }
          // put newvals into lockvals
          lockvals.insert(lockvals.end(),newvals.begin(),newvals.end());
          curNumLocked += numnew;
          // add locked vecs as aux vecs, along with aux vecs from problem
          {
            std::vector<int> indlock(curNumLocked);
            for (int i=0; i<curNumLocked; i++) indlock[i] = i;
            Teuchos::RCP<const MV> curlocked = MVT::CloneView(*lockvecs,indlock);
            if (probauxvecs != Teuchos::null) {
              lobpcg_solver->setAuxVecs( Teuchos::tuple< Teuchos::RCP<const MV> >(probauxvecs,curlocked) );
            }
            else {
              lobpcg_solver->setAuxVecs( Teuchos::tuple< Teuchos::RCP<const MV> >(curlocked) );
            }
          }
          // add locked vals to ordertest
          ordertest->setAuxVals(lockvals);
          // fill out the empty state in the solver
          {
            LOBPCGState<ScalarType,MV> state = lobpcg_solver->getState();
            Teuchos::RCP<MV> newstateX, newstateMX, newstateP, newstateMP;
            //
            // workMV will be partitioned as follows: workMV = [X P MX MP], 
            //
            // make a copy of the current X,MX state
            std::vector<int> bsind(blockSize_); 
            for (int i=0; i<blockSize_; i++) bsind[i] = i;
            newstateX = MVT::CloneViewNonConst(*workMV,bsind);
            MVT::SetBlock(*state.X,bsind,*newstateX);

            if (state.MX != Teuchos::null) {
              std::vector<int> block3(blockSize_);
              for (int i=0; i<blockSize_; i++) block3[i] = 2*blockSize_+i;
              newstateMX = MVT::CloneViewNonConst(*workMV,block3);
              MVT::SetBlock(*state.MX,bsind,*newstateMX);
            }
            //
            // get select part, set to random, apply M
            {
              Teuchos::RCP<MV> newX = MVT::CloneViewNonConst(*newstateX,indnew);
              MVT::MvRandom(*newX);

              if (newstateMX != Teuchos::null) {
                Teuchos::RCP<MV> newMX = MVT::CloneViewNonConst(*newstateMX,indnew);
                OPT::Apply(*problem_->getM(),*newX,*newMX);
              }
            }

            Teuchos::Array<Teuchos::RCP<const MV> > curauxvecs = lobpcg_solver->getAuxVecs();
            Teuchos::Array<Teuchos::RCP<Teuchos::SerialDenseMatrix<int,ScalarType> > > dummyC;
            // ortho X against the aux vectors
            ortho->projectAndNormalizeMat(*newstateX,curauxvecs,dummyC,Teuchos::null,newstateMX);

            if (hadP) {
              //
              // get P and optionally MP, orthogonalize against X and auxiliary vectors
              std::vector<int> block2(blockSize_);
              for (int i=0; i<blockSize_; i++) block2[i] = blockSize_+i;
              newstateP = MVT::CloneViewNonConst(*workMV,block2);
              MVT::SetBlock(*state.P,bsind,*newstateP);

              if (state.MP != Teuchos::null) {
                std::vector<int> block4(blockSize_);
                for (int i=0; i<blockSize_; i++) block4[i] = 3*blockSize_+i;
                newstateMP = MVT::CloneViewNonConst(*workMV,block4);
                MVT::SetBlock(*state.MP,bsind,*newstateMP);
              }

              if (fullOrtho_) {
                // ortho P against the new aux vectors and new X
                curauxvecs.push_back(newstateX);
                ortho->projectAndNormalizeMat(*newstateP,curauxvecs,dummyC,Teuchos::null,newstateMP);
              }
              else {
                // ortho P against the new aux vectors
                ortho->projectAndNormalizeMat(*newstateP,curauxvecs,dummyC,Teuchos::null,newstateMP);
              }
            }
            // set the new state
            LOBPCGState<ScalarType,MV> newstate;
            newstate.X  = newstateX;
            newstate.MX = newstateMX;
            newstate.P  = newstateP;
            newstate.MP = newstateMP;
            lobpcg_solver->initialize(newstate);
          }

          if (curNumLocked == maxLocked_) {
            // disable locking now; remove locking test from combo test
            combotest->removeTest(locktest);
          }
        }
        else {
          TEST_FOR_EXCEPTION(true,std::logic_error,"Anasazi::LOBPCGSolMgr::solve(): Invalid return from lobpcg_solver::iterate().");
        }
      }
      ////////////////////////////////////////////////////////////////////////////////////
      //
      // check Ritz Failure
      //
      ////////////////////////////////////////////////////////////////////////////////////
      catch (const LOBPCGRitzFailure &re) {
        if (fullOrtho_==true || recover_==false) {
          // if we are already using full orthogonalization, there isn't much we can do here. 
          // the most recent information in the status tests is still valid, and can be used to extract/return the 
          // eigenpairs that have converged.
          printer->stream(Warnings) << "Error! Caught LOBPCGRitzFailure at iteration " << lobpcg_solver->getNumIters() << std::endl
            << "Will not try to recover." << std::endl;
          break; // while(1)
        }
        printer->stream(Warnings) << "Error! Caught LOBPCGRitzFailure at iteration " << lobpcg_solver->getNumIters() << std::endl
          << "Full orthogonalization is off; will try to recover." << std::endl;
        // get the current "basis" from the solver, orthonormalize it, do a rayleigh-ritz, and restart with the ritz vectors
        // if there aren't enough, break and quit with what we have
        //
        // workMV = [X H P OpX OpH OpP], where OpX OpH OpP will be used for K and M
        LOBPCGState<ScalarType,MV> curstate = lobpcg_solver->getState();
        Teuchos::RCP<MV> restart, Krestart, Mrestart;
        int localsize = lobpcg_solver->hasP() ? 3*blockSize_ : 2*blockSize_;
        bool hasM = problem_->getM() != Teuchos::null;
        {
          std::vector<int> recind(localsize);
          for (int i=0; i<localsize; i++) recind[i] = i;
          restart = MVT::CloneViewNonConst(*workMV,recind);
        }
        {
          std::vector<int> recind(localsize);
          for (int i=0; i<localsize; i++) recind[i] = localsize+i;
          Krestart = MVT::CloneViewNonConst(*workMV,recind);
        }
        if (hasM) {
          Mrestart = Krestart;
        }
        else {
          Mrestart = restart;
        }
        //
        // set restart = [X H P] and Mrestart = M*[X H P]
        //
        // put X into [0 , blockSize)
        {
          std::vector<int> blk1(blockSize_);
          for (int i=0; i < blockSize_; i++) blk1[i] = i;
          MVT::SetBlock(*curstate.X,blk1,*restart);

          // put MX into [0 , blockSize)
          if (hasM) {
            MVT::SetBlock(*curstate.MX,blk1,*Mrestart);
          }
        }
        //
        // put H into [blockSize_ , 2*blockSize)
        {
          std::vector<int> blk2(blockSize_);
          for (int i=0; i < blockSize_; i++) blk2[i] = blockSize_+i;
          MVT::SetBlock(*curstate.H,blk2,*restart);

          // put MH into [blockSize_ , 2*blockSize)
          if (hasM) {
            MVT::SetBlock(*curstate.MH,blk2,*Mrestart);
          }
        }
        // optionally, put P into [2*blockSize,3*blockSize)
        if (localsize == 3*blockSize_) {
          std::vector<int> blk3(blockSize_);
          for (int i=0; i < blockSize_; i++) blk3[i] = 2*blockSize_+i;
          MVT::SetBlock(*curstate.P,blk3,*restart);

          // put MP into [2*blockSize,3*blockSize)
          if (hasM) {
            MVT::SetBlock(*curstate.MP,blk3,*Mrestart);
          }
        }
        // project against auxvecs and locked vecs, and orthonormalize the basis
        Teuchos::Array<Teuchos::RCP<Teuchos::SerialDenseMatrix<int,ScalarType> > > dummyC;
        Teuchos::Array<Teuchos::RCP<const MV> > Q;
        {
          if (curNumLocked > 0) {
            std::vector<int> indlock(curNumLocked);
            for (int i=0; i<curNumLocked; i++) indlock[i] = i;
            Teuchos::RCP<const MV> curlocked = MVT::CloneView(*lockvecs,indlock);
            Q.push_back(curlocked);
          }
          if (probauxvecs != Teuchos::null) {
            Q.push_back(probauxvecs);
          }
        }
        int rank = ortho->projectAndNormalizeMat(*restart,Q,dummyC,Teuchos::null,Mrestart);
        if (rank < blockSize_) {
          // quit
          printer->stream(Errors) << "Error! Recovered basis only rank " << rank << ". Block size is " << blockSize_ << ".\n"
            << "Recovery failed." << std::endl;
          break;
        }
        // reduce multivec size if necessary
        if (rank < localsize) {
          localsize = rank;
          std::vector<int> redind(localsize);
          for (int i=0; i<localsize; i++) redind[i] = i;
          // grab the first part of restart,Krestart
          restart = MVT::CloneViewNonConst(*restart,redind);
          Krestart = MVT::CloneViewNonConst(*Krestart,redind);
          if (hasM) {
            Mrestart = Krestart;
          }
          else {
            Mrestart = restart;
          }
        }
        Teuchos::SerialDenseMatrix<int,ScalarType> KK(localsize,localsize), MM(localsize,localsize), S(localsize,localsize);
        std::vector<MagnitudeType> theta(localsize);
        // project the matrices
        //
        // MM = restart^H M restart
        MVT::MvTransMv(1.0,*restart,*Mrestart,MM);
        // 
        // compute Krestart = K*restart
        OPT::Apply(*problem_->getOperator(),*restart,*Krestart);
        //
        // KK = restart^H K restart
        MVT::MvTransMv(1.0,*restart,*Krestart,KK);
        rank = localsize;
        msutils::directSolver(localsize,KK,Teuchos::rcpFromRef(MM),S,theta,rank,1);
        if (rank < blockSize_) {
          printer->stream(Errors) << "Error! Recovered basis of rank " << rank << " produced only " << rank << "ritz vectors.\n"
            << "Block size is " << blockSize_ << ".\n"
            << "Recovery failed." << std::endl;
          break;
        }
        theta.resize(rank);
        //
        // sort the ritz values using the sort manager
        {
          Teuchos::BLAS<int,ScalarType> blas;
          std::vector<int> order(rank);
          // sort
          sorter->sort( theta, Teuchos::rcpFromRef(order),rank );   // don't catch exception
          // Sort the primitive ritz vectors
          Teuchos::SerialDenseMatrix<int,ScalarType> curS(Teuchos::View,S,rank,rank);
          msutils::permuteVectors(order,curS);
        }
        //
        Teuchos::SerialDenseMatrix<int,ScalarType> S1(Teuchos::View,S,localsize,blockSize_);
        //
        // compute the ritz vectors: store them in Krestart
        LOBPCGState<ScalarType,MV> newstate;
        Teuchos::RCP<MV> newX; 
        {
          std::vector<int> bsind(blockSize_);
          for (int i=0; i<blockSize_; i++) bsind[i] = i;
          newX = MVT::CloneViewNonConst(*Krestart,bsind);
        }
        MVT::MvTimesMatAddMv(1.0,*restart,S1,0.0,*newX);
        // send X and theta into the solver
        newstate.X = newX;
        theta.resize(blockSize_);
        newstate.T = Teuchos::rcpFromRef(theta);
        // initialize
        lobpcg_solver->initialize(newstate);
      }
      catch (const AnasaziError &err) {
        printer->stream(Errors) 
          << "Anasazi::LOBPCGSolMgr::solve() caught unexpected exception from Anasazi::LOBPCG::iterate() at iteration " << lobpcg_solver->getNumIters() << std::endl
          << err.what() << std::endl
          << "Anasazi::LOBPCGSolMgr::solve() returning Unconverged with no solutions." << std::endl;
        return Unconverged;
      }
      // don't catch any other exceptions
    }

    sol.numVecs = ordertest->howMany();
    if (sol.numVecs > 0) {
      sol.Evecs = MVT::Clone(*problem_->getInitVec(),sol.numVecs);
      sol.Espace = sol.Evecs;
      sol.Evals.resize(sol.numVecs);
      std::vector<MagnitudeType> vals(sol.numVecs);

      // copy them into the solution
      std::vector<int> which = ordertest->whichVecs();
      // indices between [0,blockSize) refer to vectors/values in the solver
      // indices between [-curNumLocked,-1] refer to locked vectors/values [0,curNumLocked)
      // everything has already been ordered by the solver; we just have to partition the two references
      std::vector<int> inlocked(0), insolver(0);
      for (unsigned int i=0; i<which.size(); i++) {
        if (which[i] >= 0) {
          TEST_FOR_EXCEPTION(which[i] >= blockSize_,std::logic_error,"Anasazi::LOBPCGSolMgr::solve(): positive indexing mistake from ordertest.");
          insolver.push_back(which[i]);
        }
        else {
          // sanity check
          TEST_FOR_EXCEPTION(which[i] < -curNumLocked,std::logic_error,"Anasazi::LOBPCGSolMgr::solve(): negative indexing mistake from ordertest.");
          inlocked.push_back(which[i] + curNumLocked);
        }
      }

      TEST_FOR_EXCEPTION(insolver.size() + inlocked.size() != (unsigned int)sol.numVecs,std::logic_error,"Anasazi::LOBPCGSolMgr::solve(): indexing mistake.");

      // set the vecs,vals in the solution
      if (insolver.size() > 0) {
        // set vecs
        int lclnum = insolver.size();
        std::vector<int> tosol(lclnum);
        for (int i=0; i<lclnum; i++) tosol[i] = i;
        Teuchos::RCP<const MV> v = MVT::CloneView(*lobpcg_solver->getRitzVectors(),insolver);
        MVT::SetBlock(*v,tosol,*sol.Evecs);
        // set vals
        std::vector<Value<ScalarType> > fromsolver = lobpcg_solver->getRitzValues();
        for (unsigned int i=0; i<insolver.size(); i++) {
          vals[i] = fromsolver[insolver[i]].realpart;
        }
      }

      // get the vecs,vals from locked storage
      if (inlocked.size() > 0) {
        int solnum = insolver.size();
        // set vecs
        int lclnum = inlocked.size();
        std::vector<int> tosol(lclnum);
        for (int i=0; i<lclnum; i++) tosol[i] = solnum + i;
        Teuchos::RCP<const MV> v = MVT::CloneView(*lockvecs,inlocked);
        MVT::SetBlock(*v,tosol,*sol.Evecs);
        // set vals
        for (unsigned int i=0; i<inlocked.size(); i++) {
          vals[i+solnum] = lockvals[inlocked[i]];
        }
      }

      // sort the eigenvalues and permute the eigenvectors appropriately
      {
        std::vector<int> order(sol.numVecs);
        sorter->sort( vals, Teuchos::rcpFromRef(order), sol.numVecs);
        // store the values in the Eigensolution
        for (int i=0; i<sol.numVecs; i++) {
          sol.Evals[i].realpart = vals[i];
          sol.Evals[i].imagpart = MT::zero();
        }
        // now permute the eigenvectors according to order
        msutils::permuteVectors(sol.numVecs,order,*sol.Evecs);
      }

      // setup sol.index, remembering that all eigenvalues are real so that index = {0,...,0}
      sol.index.resize(sol.numVecs,0);
    }
  }

  // print final summary
  lobpcg_solver->currentStatus(printer->stream(FinalSummary));

  // print timing information
  Teuchos::TimeMonitor::summarize(printer->stream(TimingDetails));

  problem_->setSolution(sol);
  printer->stream(Debug) << "Returning " << sol.numVecs << " eigenpairs to eigenproblem." << std::endl;

  // get the number of iterations performed in this call to solve.
  numIters_ = lobpcg_solver->getNumIters();

  if (sol.numVecs < nev) {
    return Unconverged; // return from LOBPCGSolMgr::solve() 
  }
  return Converged; // return from LOBPCGSolMgr::solve() 
}


template <class ScalarType, class MV, class OP>
void 
LOBPCGSolMgr<ScalarType,MV,OP>::setGlobalStatusTest(
    const Teuchos::RCP< StatusTest<ScalarType,MV,OP> > &global) 
{
  globalTest_ = global;
}

template <class ScalarType, class MV, class OP>
const Teuchos::RCP< StatusTest<ScalarType,MV,OP> > & 
LOBPCGSolMgr<ScalarType,MV,OP>::getGlobalStatusTest() const 
{
  return globalTest_;
}

template <class ScalarType, class MV, class OP>
void 
LOBPCGSolMgr<ScalarType,MV,OP>::setDebugStatusTest(
    const Teuchos::RCP< StatusTest<ScalarType,MV,OP> > &debug)
{
  debugTest_ = debug;
}

template <class ScalarType, class MV, class OP>
const Teuchos::RCP< StatusTest<ScalarType,MV,OP> > & 
LOBPCGSolMgr<ScalarType,MV,OP>::getDebugStatusTest() const
{
  return debugTest_;
}

template <class ScalarType, class MV, class OP>
void 
LOBPCGSolMgr<ScalarType,MV,OP>::setLockingStatusTest(
    const Teuchos::RCP< StatusTest<ScalarType,MV,OP> > &locking) 
{
  lockingTest_ = locking;
}

template <class ScalarType, class MV, class OP>
const Teuchos::RCP< StatusTest<ScalarType,MV,OP> > & 
LOBPCGSolMgr<ScalarType,MV,OP>::getLockingStatusTest() const 
{
  return lockingTest_;
}

} // end Anasazi namespace

#endif /* ANASAZI_LOBPCG_SOLMGR_HPP */
