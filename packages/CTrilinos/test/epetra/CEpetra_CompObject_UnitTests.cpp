/*! \@HEADER */
/*
************************************************************************

                CTrilinos:  C interface to Trilinos
                Copyright (2009) Sandia Corporation

Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
license for use of this work by or on behalf of the U.S. Government.

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA
Questions? Contact M. Nicole Lemaster (mnlemas\@sandia.gov)

************************************************************************
*/
/*! \@HEADER */


#include "CTrilinos_config.h"
#include "CEpetra_Flops.h"
#include "CEpetra_Comm.h"
#include "Epetra_CompObject.h"
#include "CEpetra_CompObject.h"
#include "CEpetra_CompObject_Cpp.hpp"
#include "Teuchos_RCP.hpp"
#include "CTrilinos_enums.h"
#include "CTrilinos_exceptions.hpp"
#include "CTrilinos_utils.hpp"
#include "CTrilinos_utils_templ.hpp"
#include "CTrilinos_test_utils.hpp"

#include "CTrilinos_UnitTestHelpers.hpp"
#include "Teuchos_UnitTestHarness.hpp"


namespace {


/**********************************************************************
CT_Epetra_CompObject_ID_t Epetra_CompObject_Create (  );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , Create )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());

  /* Now check the result of the call to the wrapper function */
  TEST_EQUALITY(selfID.table, CT_Epetra_CompObject_ID);
  TEST_EQUALITY_CONST(selfID.index, 0);
}

/**********************************************************************
CT_Epetra_CompObject_ID_t Epetra_CompObject_Duplicate ( 
  CT_Epetra_CompObject_ID_t SourceID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , Duplicate )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());
  ECHO(CT_Epetra_CompObject_ID_t dupID = Epetra_CompObject_Duplicate(selfID));

  /* Now check the result of the call to the wrapper function */
  TEST_EQUALITY(dupID.table, CT_Epetra_CompObject_ID);
  TEST_EQUALITY_CONST(dupID.index, 1);
}

/**********************************************************************
void Epetra_CompObject_Destroy ( 
  CT_Epetra_CompObject_ID_t * selfID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , Destroy )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());
  ECHO(Epetra_CompObject_Destroy(&selfID));

  /* Now check the result of the call to the wrapper function */
  TEST_EQUALITY(selfID.table, CT_Invalid_ID);
  TEST_EQUALITY_CONST(selfID.index, -1);
}

/**********************************************************************
void Epetra_CompObject_SetFlopCounter ( 
  CT_Epetra_CompObject_ID_t selfID, 
  CT_Epetra_Flops_ID_t FlopCounter_inID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , SetFlopCounter )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());

  ECHO(CT_Epetra_Flops_ID_t fID = Epetra_Flops_Create());
  ECHO(Epetra_CompObject_SetFlopCounter(selfID, fID));

  /* Now check the result of the call to the wrapper function */
  ECHO(CT_Epetra_Flops_ID_t fID2 = Epetra_CompObject_GetFlopCounter(selfID));
  TEST_EQUALITY_CONST(CEpetra::getFlops(fID2).is_null(), false);
}

/**********************************************************************
void Epetra_CompObject_SetFlopCounter_Matching ( 
  CT_Epetra_CompObject_ID_t selfID, 
  CT_Epetra_CompObject_ID_t CompObjectID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , SetFlopCounter_Matching )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t compID = Epetra_CompObject_Create());
  ECHO(CT_Epetra_Flops_ID_t fID = Epetra_Flops_Create());
  ECHO(Epetra_CompObject_SetFlopCounter(compID, fID));

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());
  ECHO(Epetra_CompObject_SetFlopCounter_Matching(selfID, compID));

  /* Now check the result of the call to the wrapper function */
  ECHO(CT_Epetra_Flops_ID_t fID2 = Epetra_CompObject_GetFlopCounter(selfID));
  TEST_EQUALITY_CONST(CEpetra::getFlops(fID2).is_null(), false);
}

/**********************************************************************
void Epetra_CompObject_UnsetFlopCounter ( 
  CT_Epetra_CompObject_ID_t selfID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , UnsetFlopCounter )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());
  ECHO(CT_Epetra_Flops_ID_t fID = Epetra_Flops_Create());
  ECHO(Epetra_CompObject_SetFlopCounter(selfID, fID));

  ECHO(Epetra_CompObject_UnsetFlopCounter(selfID));

  /* Get an RCP to the CompObject outside of CTrilinos */
  ECHO(Teuchos::RCP<Epetra_CompObject> rcp1 = CEpetra::getCompObject(selfID));

  /* Now check the result of the call to the wrapper function */
  TEST_EQUALITY_CONST(rcp1->GetFlopCounter(), 0);
}

/**********************************************************************
CT_Epetra_Flops_ID_t Epetra_CompObject_GetFlopCounter ( 
  CT_Epetra_CompObject_ID_t selfID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , GetFlopCounter )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());
  ECHO(CT_Epetra_Flops_ID_t fID = Epetra_Flops_Create());
  ECHO(Epetra_CompObject_SetFlopCounter(selfID, fID));

  ECHO(CT_Epetra_Flops_ID_t fID2 = Epetra_CompObject_GetFlopCounter(selfID));

  /* Now check the result of the call to the wrapper function */
  TEST_EQUALITY_CONST(CEpetra::getFlops(fID2).is_null(), false);
}

/**********************************************************************
void Epetra_CompObject_ResetFlops ( 
  CT_Epetra_CompObject_ID_t selfID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , ResetFlops )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());

  ECHO(CT_Epetra_Flops_ID_t fID = Epetra_Flops_Create());
  ECHO(Epetra_CompObject_SetFlopCounter(selfID, fID));

  ECHO(int f = 10);
  ECHO(Epetra_CompObject_UpdateFlops_Int(selfID, f));
  ECHO(double f2 = Epetra_CompObject_Flops(selfID));
  TEST_EQUALITY(f2, f);

  ECHO(Epetra_CompObject_ResetFlops(selfID));

  /* Now check the result of the call to the wrapper function */
  ECHO(double f3 = Epetra_CompObject_Flops(selfID));
  TEST_EQUALITY_CONST(f3, 0.0);
}

/**********************************************************************
double Epetra_CompObject_Flops ( CT_Epetra_CompObject_ID_t selfID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , Flops )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());

  ECHO(CT_Epetra_Flops_ID_t fID = Epetra_Flops_Create());
  ECHO(Epetra_CompObject_SetFlopCounter(selfID, fID));

  ECHO(double f = 101.3);
  ECHO(Epetra_CompObject_UpdateFlops_Double(selfID, f));
  ECHO(double f2 = Epetra_CompObject_Flops(selfID));

  /* Now check the result of the call to the wrapper function */
  TEST_EQUALITY(f2, f);
}

/**********************************************************************
void Epetra_CompObject_UpdateFlops_Int ( 
  CT_Epetra_CompObject_ID_t selfID, int Flops_in );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , UpdateFlops_Int )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());

  ECHO(CT_Epetra_Flops_ID_t fID = Epetra_Flops_Create());
  ECHO(Epetra_CompObject_SetFlopCounter(selfID, fID));

  ECHO(int f = 435);
  ECHO(Epetra_CompObject_UpdateFlops_Int(selfID, f));

  /* Now check the result of the call to the wrapper function */
  ECHO(double f2 = Epetra_CompObject_Flops(selfID));
  TEST_EQUALITY(f2, f);
}

/**********************************************************************
void Epetra_CompObject_UpdateFlops_Long ( 
  CT_Epetra_CompObject_ID_t selfID, long int Flops_in );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , UpdateFlops_Long )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());

  ECHO(CT_Epetra_Flops_ID_t fID = Epetra_Flops_Create());
  ECHO(Epetra_CompObject_SetFlopCounter(selfID, fID));

  ECHO(long f = 4357654);
  ECHO(Epetra_CompObject_UpdateFlops_Long(selfID, f));

  /* Now check the result of the call to the wrapper function */
  ECHO(double f2 = Epetra_CompObject_Flops(selfID));
  TEST_EQUALITY(f2, f);
}

/**********************************************************************
void Epetra_CompObject_UpdateFlops_Double ( 
  CT_Epetra_CompObject_ID_t selfID, double Flops_in );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , UpdateFlops_Double )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());

  ECHO(CT_Epetra_Flops_ID_t fID = Epetra_Flops_Create());
  ECHO(Epetra_CompObject_SetFlopCounter(selfID, fID));

  ECHO(double f = 95.4632);
  ECHO(Epetra_CompObject_UpdateFlops_Double(selfID, f));

  /* Now check the result of the call to the wrapper function */
  ECHO(double f2 = Epetra_CompObject_Flops(selfID));
  TEST_EQUALITY(f2, f);
}

/**********************************************************************
void Epetra_CompObject_UpdateFlops_Float ( 
  CT_Epetra_CompObject_ID_t selfID, float Flops_in );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , UpdateFlops_Float )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());

  ECHO(CT_Epetra_Flops_ID_t fID = Epetra_Flops_Create());
  ECHO(Epetra_CompObject_SetFlopCounter(selfID, fID));

  ECHO(float f = 15.4F);
  ECHO(Epetra_CompObject_UpdateFlops_Float(selfID, f));

  /* Now check the result of the call to the wrapper function */
  ECHO(double f2 = Epetra_CompObject_Flops(selfID));
  TEST_EQUALITY(f2, f);
}

/**********************************************************************
void Epetra_CompObject_Assign ( 
  CT_Epetra_CompObject_ID_t selfID, CT_Epetra_CompObject_ID_t srcID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_CompObject , Assign )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_CompObject_ID_t compID = Epetra_CompObject_Create());
  ECHO(CT_Epetra_Flops_ID_t fID = Epetra_Flops_Create());
  ECHO(Epetra_CompObject_SetFlopCounter(compID, fID));

  ECHO(CT_Epetra_CompObject_ID_t selfID = Epetra_CompObject_Create());
  ECHO(Epetra_CompObject_Assign(selfID, compID));

  /* Now check the result of the call to the wrapper function */
  ECHO(CT_Epetra_Flops_ID_t fID2 = Epetra_CompObject_GetFlopCounter(selfID));
  TEST_EQUALITY_CONST(CEpetra::getFlops(fID2).is_null(), false);
}

/**********************************************************************/

//
// Template Instantiations
//


#ifdef TEUCHOS_DEBUG

#  define DEBUG_UNIT_TEST_GROUP( T ) \

#else

#  define DEBUG_UNIT_TEST_GROUP( T )

#endif


#define UNIT_TEST_GROUP( T ) \
  DEBUG_UNIT_TEST_GROUP( T )


} // namespace

