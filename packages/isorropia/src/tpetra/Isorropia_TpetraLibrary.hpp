//@HEADER
/*
************************************************************************

              Isorropia: Partitioning and Load Balancing Package
                Copyright (2006) Sandia Corporation

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

************************************************************************
*/
//@HEADER

#ifndef _Isorropia_TpetraLibrary_hpp_
#define _Isorropia_TpetraLibrary_hpp_

#include <Isorropia_ConfigDefs.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>


#ifdef HAVE_ISORROPIA_TPETRA
#include <Isorropia_TpetraCostDescriber.hpp>


namespace Isorropia {

namespace Tpetra {


/** An implementation of the Partitioner interface that operates on
    Tpetra matrices and linear systems.

*/

template <typename Node=Kokkos::DefaultNode::DefaultNodeType>
class Library {
public:

  Library(Teuchos::RCP<const ::Tpetra::CrsGraph<int,int,Node> > input_graph, int itype = unspecified_input_);
  Library(Teuchos::RCP<const ::Tpetra::CrsGraph<int,int,Node> > input_graph, 
          Teuchos::RCP<const ::Tpetra::MultiVector<double,int,int,Node> > input_coords,
          int itype = unspecified_input_);
  Library(Teuchos::RCP<const ::Tpetra::CrsGraph<int,int,Node> > input_graph,
	  Teuchos::RCP<CostDescriber<Node> > costs, int itype = unspecified_input_);
  Library(Teuchos::RCP<const ::Tpetra::CrsGraph<int,int,Node> > input_graph, Teuchos::RCP<CostDescriber<Node> > costs, 
	  Teuchos::RCP<const ::Tpetra::MultiVector<double,int,int,Node> > input_coords, 
          Teuchos::RCP<const ::Tpetra::MultiVector<double,int,int,Node> > weights,
          int itype = unspecified_input_);
  Library(Teuchos::RCP<const ::Tpetra::RowMatrix<double,int,int,Node> > input_matrix, int itype = unspecified_input_);
  Library(Teuchos::RCP<const ::Tpetra::RowMatrix<double,int,int,Node> > input_matrix, Teuchos::RCP<const ::Tpetra::MultiVector<double,int,int,Node> > input_coords,
          int itype = unspecified_input_);
  Library(Teuchos::RCP<const ::Tpetra::RowMatrix<double,int,int,Node> > input_matrix,
 	  Teuchos::RCP<CostDescriber<Node> > costs, int itype = unspecified_input_);
  Library(Teuchos::RCP<const ::Tpetra::RowMatrix<double,int,int,Node> > input_matrix, Teuchos::RCP<CostDescriber<Node> > costs, 
	  Teuchos::RCP<const ::Tpetra::MultiVector<double,int,int,Node> > input_coords, Teuchos::RCP<const ::Tpetra::MultiVector<double,int,int,Node> > weights,
          int itype = unspecified_input_);
  Library(Teuchos::RCP<const ::Tpetra::MultiVector<double,int,int,Node> > input_coords, int itype = unspecified_input_);
  Library(Teuchos::RCP<const ::Tpetra::MultiVector<double,int,int,Node> > input_coords,
          Teuchos::RCP<const ::Tpetra::MultiVector<double,int,int,Node> > weights, int itype = unspecified_input_);
  Library(Teuchos::RCP<const ::Tpetra::Map<int,int,Node> > input_map, int itype = unspecified_input_);

  virtual ~Library();

  virtual int
  repartition(Teuchos::ParameterList& paramlist,
	      std::vector<int>& myNewElements,
	      int& exportsSize,
	      std::vector<int>& imports) = 0;

  virtual int
  color(Teuchos::ParameterList& paramlist,
	std::vector<int>& colorAssignment) = 0 ;

  virtual int
  order(Teuchos::ParameterList& paramlist,
	std::vector<int>& orderAssignment) = 0 ;

  /** input_type_ == hgraph_input_
      This indicates that the matrix or graph represents a hypergraph.  Columns
      represent hyperedges, and row (vertex) partitioning is to be performed.
    */

  static const int hgraph_input_ = 1;

  /** input_type_ == hgraph2d_finegrain_input_
      This indicates that the matrix or graph represents a hypergraph.  Columns
      represent hyperedges, and non-zeroes are to be partitioned.
    */
  static const int hgraph2d_finegrain_input_ = 2;

  /** input_type_ == graph_input_ 
      This indicates that the square symmetric matrix or graph represents a graph
      in the sense that row/column IDs are vertices and non-zeroes represent
      edges.  The vertices are to be partitioned.
    */
  static const int graph_input_ = 3;

  /** input_type_ == geometric_input_
      This indicates that the ::Tpetra::MultiVector represents geometric
      coordinates.  The MultiVector should have 1, 2 or 3 vectors,
      representing 1, 2 or 3 dimensional coordinates.  The coordinates
      are to be partitioned.
    */
  static const int geometric_input_ = 4;

  /** input_type_ == hgraph_graph_input_
      This indicates that the ::Tpetra::MultiVector represents a hypergraph
      and graph (see above).  This is necessary for hierarchical partitioning
      with both hypergraph and graph methods.
    */
  static const int hgraph_graph_input_ = 5;

  /** input_type_ == hgraph_geom_input_
      This indicates that the ::Tpetra::MultiVector represents a hypergraph
      and graph (see above).  This is necessary for hierarchical partitioning
      with both hypergraph and geometric methods.
    */
  static const int hgraph_geometric_input_ = 6;

  /** input_type_ == graph_geom_input_
      This indicates that the ::Tpetra::MultiVector represents a hypergraph
      and graph (see above).  This is necessary for hierarchical partitioning
      with both graph and geometric methods.
    */
  static const int graph_geometric_input_ = 7;

  /** input_type_ == hgraph_graph_geom_input_
      This indicates that the ::Tpetra::MultiVector represents a hypergraph
      and graph (see above).  This is necessary for hierarchical partitioning
      using hypergraph, graph, and geometric methods.
    */
  static const int hgraph_graph_geometric_input_ = 8;


  /** input_type_ == simple_input_
      This is used to indicate that a simple partitiong method
      (block, cyclic, or random) will be used.
    */
  static const int simple_input_ = 9;


  /** input_type_ == unspecified_input_ 
      This value is the "unset" state for the input_type_ instance variable.
    */

  static const int unspecified_input_ = 10;

  int input_type_;

  int numPartSizes;
  int *partGIDs;
  float *partSizes;

protected:

  Teuchos::RCP<const ::Tpetra::Map<int,int,Node> > input_map_;
  Teuchos::RCP<const ::Tpetra::CrsGraph<int,int,Node> > input_graph_;
  Teuchos::RCP<const ::Tpetra::RowMatrix<double,int,int,Node> > input_matrix_;
  Teuchos::RCP<const ::Tpetra::MultiVector<double,int,int,Node> > input_coords_;
  Teuchos::RCP<Isorropia::Tpetra::CostDescriber<Node> > costs_;
  Teuchos::RCP<const ::Tpetra::MultiVector<double,int,int,Node> > weights_;

  virtual int precompute();

  virtual int postcompute() = 0;

};//class Library

}//namespace Tpetra
}//namespace Isorropia

#endif //HAVE_ISORROPIA_TPETRA

#endif

