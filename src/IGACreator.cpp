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

#include "iga/IGACreator.h"
#include <algorithm>

namespace iga_fileio
{
	bool CoeffVectorLessThan::operator()( const CoeffVector &a, const CoeffVector &b ) const
	{
		return std::lexicographical_compare( a.begin(), a.end(), b.begin(), b.end() );
	}

	IGACreator::IGACreator( IGAData *parent ) : mParent( parent )
	{
		parent->clear();
	}

	// Used by several of the IGACreator member functions. This appends to a vector
	// with some guards against 32-bit integer overflow, as we use 32-bit integers
	// for our indexing.
	template< typename T >
	uint32_t safeAppend( std::vector< T > &vec, const T &value )
	{
		// Guard against various forms of overflow, returning an error code if any occurs.
		if( vec.size() >= INVALID_INDEX - 1 || vec.size() >= vec.max_size() )
			return INVALID_INDEX;

		// Add the value to our vector and return the index created.
		uint32_t added_index = static_cast< unsigned >( vec.size() );
		vec.push_back( value );
		return added_index;
	}

	uint32_t IGACreator::addCoeffs( const CoeffVector &coeffs )
	{
		// Alias to the mCoeffs in mParent.
		auto &mCoeffs = mParent->mCoeffs;

		// I can't use the safeAppend template for this one because I'm appending
		// a vector to the vector. This guards against the more complex forms of
		// integer overflow that are possible in that case. The check against 0x7FFF
		// guards against appending unrepresentable orders; reasonable orders are
		// much lower.
		if( mCoeffs.size() + coeffs.size() >= INVALID_INDEX ||
			mCoeffs.size() + coeffs.size() > mCoeffs.max_size() ||
			coeffs.size() >= 0x7FFF )
			return INVALID_INDEX;

		// Add these coefficients to our coefficient array and return the index used.
		uint32_t dict_index = static_cast< unsigned >( mCoeffs.size() );
		mCoeffs.insert( mCoeffs.end(), coeffs.begin(), coeffs.end() );
		return dict_index;
	}

	uint32_t IGACreator::addEdge( uint32_t elem, double knot_interval )
	{
		// Alias to the member variables in the parent.
		auto &mEdges = mParent->mEdges;
		auto &mIntervals = mParent->mIntervals;

		uint32_t edge_index = safeAppend( mEdges, elem );
		if( knot_interval >= 0.0 )
		{
			uint32_t interval_index = safeAppend( mIntervals, knot_interval );
			// You must keep these in sync.
			if( edge_index != interval_index )
				return INVALID_INDEX;
		}
		else if( !mIntervals.empty() )
			return INVALID_INDEX;
		return edge_index;
	}

	uint32_t IGACreator::addElem( const Elem &elem )
	{
		return safeAppend( mParent->mElems, elem );
	}

	uint32_t IGACreator::addExplicitPiece( int s_order, uint32_t pt_index, const CoeffVector &coeffs )
	{
		if( coeffs.size() > 0x7FFF * 0x7FFF )
			return INVALID_INDEX;
		int t_order = static_cast< int >( coeffs.size() ) / s_order;
		if( s_order < 0 || s_order > 0x7FFF ||
			t_order < 0 || t_order > 0x7FFF )
			return INVALID_INDEX;

		// The static_casts should be unnecessary, as we checked the range already,
		// but can help avoid compiler warnings about arithmetic overflow.
		if( static_cast< size_t >( s_order ) * static_cast< size_t >( t_order ) != coeffs.size() )
			return INVALID_INDEX;

		Piece2D p;
		p.st_order = ( s_order ) | ( t_order << 16 );
		p.s_index = getDictionaryIndex( coeffs );
		if( p.s_index == INVALID_INDEX )
			return INVALID_INDEX;
		p.maybe_t_index = INVALID_INDEX;
		p.pt_index = pt_index;
		return addPiece( p );
	}

	uint32_t IGACreator::addLayout( const FaceLayout &layout )
	{
		return safeAppend( mParent->mLayouts, layout );
	}

	uint32_t IGACreator::addPiece( const Piece2D &piece )
	{
		return safeAppend( mParent->mPieces, piece );
	}

	uint32_t IGACreator::addPoint( const Point3d &pt )
	{
		return safeAppend( mParent->mPoints, pt );
	}

	uint32_t IGACreator::addTensorPiece( const CoeffVector &s_coeffs, const CoeffVector &t_coeffs, uint32_t pt_index )
	{
		int s_order = static_cast< int >( s_coeffs.size() );
		int t_order = static_cast< int >( t_coeffs.size() );
		if( s_order < 0 || s_order > 0x7FFF ||
			t_order < 0 || t_order > 0x7FFF )
			return INVALID_INDEX;

		Piece2D p;
		p.st_order = ( s_order ) | ( t_order << 16 );
		p.s_index = getDictionaryIndex( s_coeffs );
		if( p.s_index == INVALID_INDEX )
			return INVALID_INDEX;
		p.maybe_t_index = getDictionaryIndex( t_coeffs );
		if( p.maybe_t_index == INVALID_INDEX )
			return INVALID_INDEX;
		p.pt_index = pt_index;
		return addPiece( p );
	}

	uint32_t IGACreator::finishElem( uint32_t layout_index )
	{
		Elem elem;

		elem.edge_end_index = mParent->edgeCount();
		elem.layout_index = layout_index;
		elem.piece_end_index = mParent->pieceCount();

		// Check to make sure the caller didn't make a layout mistake.
		auto &mLayouts = mParent->mLayouts;
		if( layout_index >= mLayouts.size() )
			return INVALID_INDEX;
		const FaceLayout &layout = mLayouts[ layout_index ];
		uint32_t elem_edge_count = [&]() {
			if( mParent->mElems.empty() )
				return static_cast< uint32_t >( mParent->mEdges.size() );
			return static_cast< uint32_t >( mParent->mEdges.size() - mParent->mElems.back().edge_end_index );
		}();
		if( elem_edge_count != layout.side_range[ 4 ] )
			return INVALID_INDEX;

		return addElem( elem );
	}

	uint32_t IGACreator::getDictionaryIndex( const CoeffVector &coeffs )
	{
		// Ensure that these coefficients contain no bad data. No infinities,
		// and no NAN values (NAN in particular will cause issues with our
		// lookup table).
		for( double i : coeffs )
			if( !finite( i ) )
				return INVALID_INDEX;

		// Do we already have an entry for these coeffs? It must be an exact match.
		auto iter = mCoeffLookup.find( coeffs );
		if( iter != mCoeffLookup.end() )
			return iter->second;

		uint32_t new_index = addCoeffs( coeffs );
		if( new_index == INVALID_INDEX )
			return INVALID_INDEX;
		mCoeffLookup[ coeffs ] = new_index;
		return new_index;
	}

	uint32_t IGACreator::getLayoutIndex( const FaceLayout &layout )
	{
		auto iter = mLayoutLookup.find( layout );
		if( iter != mLayoutLookup.end() )
			return iter->second;

		// Enforce the rule that layout 0 always be the uniform layout. That is, if we're
		// adding our first layout, and it isn't the default layout, add the default layout
		// before adding the current layout.
		FaceLayout default_layout;
		if( mParent->mLayouts.empty() && ( layout < default_layout || default_layout < layout ) )
		{
			mLayoutLookup[ default_layout ] = 0;
			mParent->mLayouts.push_back( default_layout );
		}

		uint32_t new_index = addLayout( layout );
		if( new_index == INVALID_INDEX )
			return INVALID_INDEX;
		mLayoutLookup[ layout ] = new_index;
		return new_index;
	}

	void IGACreator::setSurfaceType( const std::string &surface_type )
	{
		mParent->mSrfType = surface_type;
	}
}
