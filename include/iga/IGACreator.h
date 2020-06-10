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

#ifndef IGA_CREATOR_H_
#define IGA_CREATOR_H_

#include "IGAData.h"
#include <map>

namespace iga_fileio
{
	struct CoeffVectorLessThan
	{
		bool operator()( const CoeffVector &a, const CoeffVector &b ) const;
	};

	/// Type used for coefficient vector lookup tables.
	using CoeffLookup = std::map< CoeffVector, uint32_t, CoeffVectorLessThan >;

	/// Type used for face layout lookup tables.
	using LayoutLookup = std::map< FaceLayout, uint32_t >;

	/// Use this class to add data to an IGAData object. This class is used
	/// to hold the lookup tables needed to build the dictionaries, which
	/// are used to avoid storing redundant data in an IGA file.
	class IGACreator
	{
	public:
		/// This constructor clears the parent IGAData. It would probably
		/// be possible to rebuild the lookup tables for the parent IGAData and
		/// thus allow editing of existing ones, but we don't need that currently.
		IGACreator( IGAData *parent );

		/// Add a vector of coefficients and returns the (first) index added.
		/// The vector occupies the following coeffs.size() indices. Returns
		/// INVALID_INDEX if the operation fails.
		uint32_t addCoeffs( const CoeffVector &coeffs );

		/// Add an edge and its interval. Pass a negative number for the
		/// knot_interval if you don't want to save intervals, or pass a value
		/// >= 0.0 if you do. You must be consistent about this, and it will
		/// return INVALID_INDEX (failure) if you aren't.
		uint32_t addEdge( uint32_t elem, double knot_interval );

		/// Adds an Elem and returns the index added. Returns INVALID_INDEX if
		/// the operation fails.
		uint32_t addElem( const Elem &elem );

		/// Adds a single explicit piece to the end of the data vector. Note that
		/// all pieces for a given patch must be added together. The t_order can
		/// be inferred from the size of coeffs.
		uint32_t addExplicitPiece( int s_order, uint32_t pt_index, const CoeffVector &coeffs );

		/// Adds a FaceLayout and returns the index added. Returns INVALID_INDEX
		/// if the operation fails.
		uint32_t addLayout( const FaceLayout &layout );

		/// Adds a Piece2D and returns the index added. Returns INVALID_INDEX if
		/// the operation fails. This requires you to build the Piece2D data
		/// yourself; use addExplicitPiece or addTensorPiece to avoid that.
		uint32_t addPiece( const Piece2D &piece );

		/// Adds a Point3d and returns the index added. Returns INVALID_INDEX if
		/// the operation fails.
		uint32_t addPoint( const Point3d &pt );

		// Must implement this before I'm done.
		uint32_t addTensorPiece( const CoeffVector &s_coeffs, const CoeffVector &t_coeffs, uint32_t pt_index );

		/// Call this after you've finished adding all the pieces and edges for
		/// the current element. Pass the index of the layout being used, which must
		/// be consistent with the number of edges which were added (otherwise it
		/// will return INVALID_INDEX). The index of the added element is returned.
		uint32_t finishElem( uint32_t layout_index );

		/// Given the coefficient vector 'coeffs', get an index in mCoeffDictionary
		/// that can be used to represent the start of that coefficient block.
		uint32_t getDictionaryIndex( const CoeffVector &coeffs );

		/// Given a face layout, returns the index used for representing that layout in
		/// the layout dictionary. Adds the layout if it didn't already have an index.
		uint32_t getLayoutIndex( const FaceLayout &layout );

		/// Set a string to record the type of surface that's being saved.
		void setSurfaceType( const std::string &surface_type );

	private:
		/// A "parent" IGAData object. This is the object to which we are
		/// adding data.
		IGAData *mParent = nullptr;

		/// A lookup table used to add new vectors to the coeff dictionary.
		CoeffLookup mCoeffLookup;

		/// A lookup table for the face layouts.
		LayoutLookup mLayoutLookup;
	};
}

#endif
