// Copyright 2020 Autodesk, Inc.
// 
// Licensed under the Apache License, Version 2.0 ( the "License" );
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef IGA_DATA_H_
#define IGA_DATA_H_

#include "IGACommon.h"
#include <string>

namespace iga_fileio
{
	/// 3d points in Grassmann space
	struct Point3d
	{
		double x = 0, y = 0, z = 0, w = 0;
	};

	/// Represents a single 3d point's contribution to a given element.
	struct Piece2D
	{
		/// The s_order bitwise-or'd with the t_order, with s_order being
		/// in the bottom 16 bits and the t_order being in the top.
		uint32_t st_order = 0;
		/// Index of the curve in S, unless it's explicit, in which case
		/// this is the index of the entire explicit patch.
		uint32_t s_index = 0;
		/// If this is INVALID_INDEX, the patch is explicit. Otherwise,
		/// index of the curve in T.
		uint32_t maybe_t_index = 0;
		/// Index of the Point3d holding the geometry for this piece.
		uint32_t pt_index = 0;
	};

	/// The T-junction layout for a given element's neighbors
	struct FaceLayout
	{
		uint32_t side_range[ 5 ] = { 0, 1, 2, 3, 4 };
		bool operator<( const FaceLayout &rhs ) const;
	};

	/// All the information about a single element--topology, influence,
	/// parametric dimensions, and neighbors.
	struct Elem
	{
		uint32_t piece_end_index = 0;
		uint32_t layout_index = 0;
		uint32_t edge_end_index = 0;
	};

	/// A class that represents in memory that data held in an IGA file. This class
	/// only contains getter methods and a simple clear() function. The setter
	/// methods are in IGACreator.
	///
	/// Most of the methods in this class require that you pass valid, existing
	/// indices; they do not do much bounds checking or error checking. This is to
	/// ensure good performance. It is recommended that you run error checking
	/// yourself before using this data-structure, as misuse can result in
	/// undefined behavior. See the isValid() member function for assistance
	class IGAData
	{
	public:
		/// Clear the contents of this IGAData.
		void clear();

		/// A const reference to the coefficient vector.
		const std::vector< double > &coeffs() const { return mCoeffs; }

		/// A const reference to the edges vector.
		const std::vector< uint32_t > &edges() const { return mEdges; }

		/// The starting edge index for the given Elem. Pairs with edgeEnd to let
		/// you iterate over the edges. The values between [edgeBegin..endEnd) can be used
		/// as inputs to functions that take an edge_index.
		///
		/// See also sideBegin() and sideEnd() if you wish to iterate over the edges
		/// on a particular side of the element.
		uint32_t edgeBegin( uint32_t elem_index ) const;

		/// The total number of edges stored in this IGAData.
		uint32_t edgeCount() const;

		/// The end sentinel for the edge list for the given Elem. Pairs with edgeBegin.
		/// Returns INVALID_INDEX if you specify an illegal element index.
		uint32_t edgeEnd( uint32_t elem_index ) const;

		/// Returns the length (knot interval) of the given edge. This will return 1.0 if
		/// no intervals were provided on this surface.
		double edgeInterval( uint32_t edge_index ) const;

		/// Returns the index of the adjacent element across the edge given by edge_index
		/// (which must be in the range edgeBegin >= edge_index > edgeEnd). The edge only
		/// knows about the adjacent element, it doesn't know what its own element is.
		uint32_t edgeOther( uint32_t edge_index ) const;

		/// Lets you get a reference to all of the held element data. Useful for
		/// copying or some types of iteration.
		const std::vector< Elem > &elems() const { return mElems; }

		/// Returns the number of stored elems. Equivalent to elems().size(), but with
		/// the type downcast to a 32-bit type.
		uint32_t elemCount() const;

		/// The number of edges on a particular element.
		uint32_t elemEdgeCount( uint32_t elem_index ) const;

		/// The number of edges on a particular side of an element. Sides must be in
		/// the range 0..3 where 0 = bottom, 1 = right, 2 = top, 3 = left.
		uint32_t elemEdgesOnSide( uint32_t elem_index, int side ) const;

		/// Lets you get a reference to all of the held knot interval data, if any. Note
		/// that it might be empty if the surface is fully uniform.
		const std::vector< double > &intervals() const { return mIntervals; }

		/// A version of isValid() that prints out diagnostic messages if any problems are
		/// found. It will print nothing if true is returned, which indicates success; otherwise,
		/// a diagnostic message will be printed to the output stream given in 'err'.
		bool isValid( std::ostream &err ) const;

		/// Returns true if the data in this IGAData is self-consistent; that is,
		/// all of the internal indices refer to legal values. You should generally run this
		/// after creating or loading an IGAData to ensure that you haven't made any mistakes.
		///
		/// If you wish to see diagnostic messages, use the version of this function that takes
		/// an ostream. This function returns true if all data appears to be self-consistent.
		bool isValid() const;

		/// Returns the layout structure for the given layout_index. Even if there are
		/// no layouts stored, passing 0 guarantees that the default layout
		/// { 0, 1, 2, 3, 4 } will be returned.
		const FaceLayout &layout( uint32_t layout_index ) const;

		/// The index of the given element's layout, which may be passed to functions
		/// which need a layout_index.
		uint32_t layoutIndex( uint32_t elem_index ) const;

		const std::vector< FaceLayout > &layouts() const { return mLayouts; }

		/// The index of the first piece belonging to the given element. Returns
		/// INVALID_INDEX if you provide an invalid elem_index. The values from
		/// [pieceBegin..pieceEnd) can be passed to functions that take a
		/// piece_index.
		uint32_t pieceBegin( uint32_t elem_index ) const;

		/// The total number of stored pieces. This will be the next index to be
		/// added by IGACreator::addPiece.
		uint32_t pieceCount() const;

		/// The end marker for the last piece owned by elem_index. Pairs with
		/// pieceBegin().
		uint32_t pieceEnd( uint32_t elem_index ) const;

		/// Returns a pointer to the explicit coefficients. The piece_index must be
		/// valid, and the piece must be explicit.
		const double *pieceExplicitCoeffs( uint32_t piece_index ) const;

		/// Returns true if the given piece uses explicit coefficients, i.e. it is
		/// not tensor product. The piece_index must be valid.
		bool pieceIsExplicit( uint32_t piece_index ) const;

		/// Returns true if the piece is tensor-product. The piece_index must be valid.
		bool pieceIsTensor( uint32_t piece_index ) const;

		/// Returns a reference to the geometry of the point owned by the given
		/// piece_index, which must be valid and must have a valid point.
		const Point3d &piecePoint( uint32_t piece_index ) const;

		/// The index of the point belonging to the given piece.
		uint32_t piecePointIndex( uint32_t piece_index ) const;

		/// A const reference to the pieces vector.
		const std::vector< Piece2D > &pieces() const { return mPieces; }

		/// If the piece is tensor-product and the piece_index is valid, this will
		/// return the index of the start of the SCoeffs in the coeffs() vector.
		const uint32_t pieceSIndex( uint32_t piece_index ) const;

		/// If the piece is tensor-product and the piece_index is valid, this will
		/// return a pointer to the Bernstein-basis coefficients in S. This function
		/// can crash/have UB otherwise, so check the parameters. The returned
		/// pointer is to an array whose length is retrieved using pieceSOrder.
		const double *pieceSCoeffs( uint32_t piece_index ) const;

		/// Returns the S-order of the given piece's influence. The piece_index must
		/// be valid.
		int pieceSOrder( uint32_t piece_index ) const;

		/// If the piece is tensor-product and the piece_index is valid, this will
		/// return the index of the start of the TCoeffs in the coeffs() vector.
		const uint32_t pieceTIndex( uint32_t piece_index ) const;

		/// If the piece is tensor-product and the piece_index is valid, this will
		/// return a pointer to the Bernstein-basis coefficients in T. Otherwise, this
		/// function may invoke UB, so check your parameters beforehand.
		const double *pieceTCoeffs( uint32_t piece_index ) const;

		/// Returns the T-order of the given piece's influence. The piece_index must
		/// be valid.
		int pieceTOrder( uint32_t piece_index ) const;

		/// The total number of stored points. This will be the next index to be
		/// added by IGACreator::addPoint.
		uint32_t pointCount() const;

		/// A const reference to the points vector.
		const std::vector< Point3d > &points() const { return mPoints; }

		/// Returns an edge_index that lets you iterate over the edges on a particular
		/// side of a face. The range of the edge_index is [sideBegin..sideEnd),
		/// and you may pass it to any of the functions that take an edge_index.
		///
		/// This range will be a sub-range within [edgeBegin..edgeEnd).
		uint32_t sideBegin( uint32_t elem_index, int side ) const;

		/// The counterpart to sideBegin().
		uint32_t sideEnd( uint32_t elem_index, int side ) const;

		/// Returns a reference to the string which holds the saved surface type.
		/// The default value is "unknown."
		const std::string &surfaceType() const { return mSrfType; }

	private:
		/// A simple, null-terminated, stored string to represent the
		/// surface type.
		std::string mSrfType{ "unknown" };

		/// The coefficient dictionary stored in the file (jagged 2D array).
		/// Referenced by the pieces.
		std::vector< double > mCoeffs;

		/// The control point geometry. Referenced by the pieces.
		std::vector< Point3d > mPoints;

		/// The pieces of influence for the elements (jagged 2D array).
		std::vector< Piece2D > mPieces;

		/// The elements adjacent to some current element (jagged 2D array,
		/// indexed from an element).
		std::vector< uint32_t > mEdges;

		/// Runs parallel to edges; this contains the intervals for
		/// each edge, OR it is empty, in which case all the edges have
		/// an interval of 1.0.
		std::vector< double > mIntervals;

		/// A vector of face layouts. Referenced by the elements.
		std::vector< FaceLayout > mLayouts;

		/// The top-level elements, which assemble all the other data types.
		/// Each element references an array of pieces, an array of edges,
		/// and a face layout.
		std::vector< Elem > mElems;

		/// The IGACreator has all the functions which write to this class.
		friend class IGACreator;

		/// The IGAReader has the functions for loading this class from a file.
		/// It requires direct access to the vector.
		friend class IGAReader;
	};
}

#endif
