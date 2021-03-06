/* @HEADER@ */
// ************************************************************************
// 
//                              Sundance
//                 Copyright (2005) Sandia Corporation
// 
// Copyright (year first published) Sandia Corporation.  Under the terms 
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
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
// Questions? Contact Kevin Long (krlong@sandia.gov), 
// Sandia National Laboratories, Livermore, California, USA
// 
// ************************************************************************
/* @HEADER@ */

#ifndef SUNDANCE_MIXEDDOFMAPHN_H
#define SUNDANCE_MIXEDDOFMAPHN_H


#include "SundanceDefs.hpp"
#include "SundanceMesh.hpp"
#include "SundanceCellSet.hpp"
#include "SundanceCellFilter.hpp"
#include "SundanceHNDoFMapBase.hpp"
#include "SundanceBasisFamilyBase.hpp"
#include "SundanceMatrixStore.hpp"

namespace Sundance
{
using namespace Teuchos;

class BasisDOFTopologyBase;

/** 
 * A MixedDOFMapHN is a DOF map for the case where
 * every function is defined
 * on every cell in the mesh, but where functions may have different bases. <br>
 * MixedDOFMapHN is defined on meshes with hanging nodes,
 * it builds the restriction matrix where it is needed.
 */
class MixedDOFMapHN : public HNDoFMapBase
{
public:
  /** */
	MixedDOFMapHN(const Mesh& mesh,
    const Array<RCP<BasisDOFTopologyBase> >& basis,
    const CellFilter& maxCells, 
    int setupVerb);
                        
  /** */
  virtual ~MixedDOFMapHN(){;}

  /** */
  RCP<const MapStructure> 
  getDOFsForCellBatch(int cellDim,
    const Array<int>& cellLID,
    const Set<int>& requestedFuncSet,
    Array<Array<int> >& dofs,
    Array<int>& nNodes,
    int verbosity) const ;

	/** Returns the
	 * @param cellLID [in] the maxCell LID input
	 * @param funcID [in]
	 * @param trafoMatrixSize [in/out]
	 * @param doTransform [out]
	 * @param transfMatrix [out] row major matrix*/
  void getTrafoMatrixForCell(
	    int cellLID,
	    int funcID,
	    int& trafoMatrixSize,
	    bool& doTransform,
	    Array<double>& transfMatrix ) const;

	/** Function to apply transformation for facets
	 * @param cellDim , the facet dimension
	 * @param cellLID , facet LID
	 * @param facetIndex , facet index in the maxCofacet
	 * @param funcID  [in] the function ID
	 * @param trafoMatrixSize [in/out]
	 * @param doTransform [out]
	 * @param transfMatrix [out] (we assume that the array is already pre-sized )*/
     void getTrafoMatrixForFacet(
		  int cellDim,
		  int cellLID,
		  int facetIndex,
		  int funcID,
		  int& trafoMatrixSize,
		  bool& doTransform,
		  Array<double>& transfMatrix ) const;

  /** See subclass for docu
   * We can be sure that this is used only for nodal data (for VTK plotting)<br>
   * This is implemented only for Nodal data call !!! */
  void getDOFsForHNCell(
	  int cellDim,
	  int cellLID,
      int funcID,
      Array<int>& dofs ,
      Array<double>& coefs ) const;

  /** */
  RCP<const MapStructure> mapStruct() const 
    {return structure_;}

  /** */
  int chunkForFuncID(int funcID) const
    {return structure_->chunkForFuncID(funcID);}

  /** */
  int indexForFuncID(int funcID) const 
    {return structure_->indexForFuncID(funcID);}
      
  /** */
  int nFuncs(int basisChunk) const
    {return nFuncs_[basisChunk];}

  /** */
  int nBasisChunks() const 
    {return nFuncs_.size();}

  /** */
  const RCP<BasisDOFTopologyBase>& basis(int basisChunk) const
    {return structure_->basis(basisChunk);}

  /** */
  const Array<int>& funcID(int basisChunk) const 
    {return structure_->funcs(basisChunk);}


private:

  /** */
  void checkTable() const ;

  /** */
  inline int getInitialDOFForCell(int cellDim, int cellLID, int basisChunk) const
    {
      return dofs_[cellDim][basisChunk][cellLID*nDofsPerCell_[basisChunk][cellDim]];
    }

  inline int* getInitialDOFPtrForCell(int cellDim, int cellLID, int basisChunk)
    {
      return &(dofs_[cellDim][basisChunk][cellLID*nDofsPerCell_[basisChunk][cellDim]]);
    }

  inline const int* getInitialDOFPtrForCell(int cellDim, int cellLID, 
    int basisChunk) const 
    {
      return &(dofs_[cellDim][basisChunk][cellLID*nDofsPerCell_[basisChunk][cellDim]]);
    }

  /** */
  void allocate(const Mesh& mesh);
      
  /** */
  void buildMaximalDofTable();

  bool hasBeenAssigned(int cellDim, int cellLID) const 
    {return hasBeenAssigned_[cellDim][cellLID];}

  void markAsAssigned(int cellDim, int cellLID)
    {hasBeenAssigned_[cellDim][cellLID] = true;}

  /** */
  void initMap();

  /** */
  void setDOFs(int basisChunk, int cellDim, int cellLID, 
    int& nextDOF, bool isRemote=false);

  /** */
  void shareDOFs(int cellDim,
    const Array<Array<int> >& outgoingCellRequests);

  /** */
  void computeOffsets(int dim, int localCount);

  /** */
  static int uninitializedVal() {return -1;}

  /** */
  CellFilter maxCells_;

  /** spatial dimension */
  int dim_;

  /** Tables of DOFs, indexed by dimension and chunk number.
   *
   * dof(cellDim, cellLID, chunk, func, node) 
   * = dofs_[cellDim][chunk][(cellLID*nFunc + func)*nNode + node]
   */
  Array<Array<Array<int> > > dofs_;

  /** DOFs for maximal cells, indexed by basis chunk number 
   *
   * dof(cellLID, chunk, func, node) 
   * = maximalDofs_[chunk][(cellLID*nFunc + func)*nNode + node];
   */
  mutable Array<Array<int> > maximalDofs_;

  /** whether maximal DOFs have been tabulated */
  mutable bool haveMaximalDofs_;

  /** 
   * localNodePtrs_[basisChunk][cellDim][facetDim][facetNumber][nodeNumber]
   */
  Array<Array<Array<Array<Array<int> > > > > localNodePtrs_;

/* ========= Hanging node treatment ============= */

   /** Nr of points, needed only by plotting */
   int nPoints_;

   /** Is true if the cell has hanging node */
   Array<bool> hasCellHanging_;

   /** Flag per element showing if the element is a hanging element*/
   Array< Array<bool> > isElementHanging_;

   /** store the facetLIDs of the points [nPoints*b + facetID]*/
   Sundance::Map< int , Array<int>  > HN_To_globalFacetsLID_;

   /** store the facetDim of the points [nPoints*b + facetID]*/
   Sundance::Map< int , Array<int>  > HN_To_globalFacetsDim_;

   /** store the coefs [nPoints*b + facetID]*/
   Sundance::Map< int , Array<double>  > HN_To_coeffs_;

   /** Maps one maxCell LID to one transformation matrix, only maxDim elements and basisChunck ID
    * with hanging nodes should have transformation matrix
    * [maxCellLID].get -> [basisChunckID][] is the index of trafo matrix */
   Sundance::Map< int , Array < int > > maxCellLIDwithHN_to_TrafoMatrix_;

   /** The object to store all the transformation matrixes */
   MatrixStore                          matrixStore_;

   /** The basis for the transformation */
   mutable Array<RCP<BasisFamilyBase> > basis_;

/* ========= END hanging node treatment ======== */

  /** The number of nodes per cell, for each basis function type, 
   * <i>not</i> including the nodes of the facets of the cell. Indexed as 
   * nNodesPerCell_[basis][dimension] */
  Array<Array<int> > nNodesPerCell_;

  /** The number of DOFs per cell, for each basis function type, 
   * <i>not</i> including the DOFs of the facets of the cell. Indexed as 
   * nDofsPerCell_[basis][dimension] */
  Array<Array<int> > nDofsPerCell_;

  /** The number of nodes per cell, for each basis function type, including
   * the nodes of the facets of the cell. Indexed as 
   * nNodesPerCell_[basis][dimension] */
  Array<Array<int> > totalNNodesPerCell_;

  /** The number of DOFs per cell, for each basis function type, including
   * the DOFs of the facets of the cell. Indexed as 
   * nDofsPerCell_[basis][dimension] */
  Array<Array<int> > totalNDofsPerCell_;

  /** Indicates whether the cells of each dimension have any DOFs in this
   * map, for any chunk. */
  Array<int> cellHasAnyDOFs_;

  /** number of facets of dimension facetDim for cells of dimension cellDim.
   * Indexed as numFacets_[cellDim][facetDim]
   */
  Array<Array<int> > numFacets_;

  /** Orientation of each edge or face as seen by the maximal cell
   * from which its DOFs were originally assigned. */
  Array<Array<int> > originalFacetOrientation_;

  /** */
  Array<Array<int> > hasBeenAssigned_;

  /** */
  RCP<const MapStructure> structure_;

  /** */
  Array<int> nFuncs_;
};
}

                  
#endif
