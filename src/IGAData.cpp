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

#include "iga/IGAData.h"
#include <iostream>
#include <algorithm>
#include <set>

namespace iga_fileio
{
	bool FaceLayout::operator<( const FaceLayout &rhs ) const
	{
		return std::lexicographical_compare( side_range, side_range + 5, rhs.side_range, rhs.side_range + 5 );
	}

	void IGAData::clear()
	{
		// Default assignment operator does the right thing.
		*this = IGAData();
	}

	uint32_t IGAData::edgeBegin( uint32_t elem_index ) const
	{
		if( elem_index >= mElems.size() )
			return INVALID_INDEX;

		if( elem_index == 0 )
			return 0;

		return mElems[ elem_index - 1 ].edge_end_index;
	}

	uint32_t IGAData::edgeCount() const
	{
		return static_cast< uint32_t >( mEdges.size() );
	}

	uint32_t IGAData::edgeEnd( uint32_t elem_index ) const
	{
		if( elem_index >= mElems.size() )
			return INVALID_INDEX;

		return mElems[ elem_index ].edge_end_index;
	}

	double IGAData::edgeInterval( uint32_t edge_index ) const
	{
		// Implement the requirement that if no intervals are stored, the surface
		// be treated as uniform.
		if( mIntervals.empty() )
			return 1.0;

		return mIntervals[ edge_index ];
	}

	uint32_t IGAData::edgeOther( uint32_t edge_index ) const
	{
		return mEdges[ edge_index ];
	}

	uint32_t IGAData::elemCount() const
	{
		return static_cast< uint32_t >( mElems.size() );
	}

	uint32_t IGAData::elemEdgeCount( uint32_t elem_index ) const
	{
		const FaceLayout &elem_layout = layout( mElems[ elem_index ].layout_index );
		return elem_layout.side_range[ 4 ];
	}

	uint32_t IGAData::elemEdgesOnSide( uint32_t elem_index, int side ) const
	{
		const FaceLayout &elem_layout = layout( mElems[ elem_index ].layout_index );
		return elem_layout.side_range[ side + 1 ] - elem_layout.side_range[ side ];
	}

	bool IGAData::isValid( std::ostream &err ) const
	{
		using std::endl;
		// Used to check for monotonically increasing indices.
		uint32_t last_edge_end = 0u;
		uint32_t last_piece_end = 0u;
		// Loop through all the coefficients
		for( size_t icoeff = 0; icoeff < mCoeffs.size(); ++icoeff )
		{
			if( !finite( mCoeffs[ icoeff ] ) )
			{
				err << "Coeff " << icoeff << " is not finite or is Not A Number" << endl;
				return false;
			}
		}
		for( size_t ipoint = 0; ipoint < mPoints.size(); ++ipoint )
		{
			const Point3d &pt = mPoints[ ipoint ];
			if( !finite( pt.x ) || !finite( pt.y ) || !finite( pt.z ) || !finite( pt.w ) )
			{
				err << "Point " << ipoint << " has non-finite/NAN values." << endl;
				return false;
			}
			// I'm not checking for 0 weights on the points, although those are typically
			// illegal. If you do choose to check for 0 weights, be aware that (0,0,0,0)
			// is a magic value for unused point indices, and is permitted.
		}
		// Loop through all the layouts
		// Loop through all the layouts
		std::set< FaceLayout > layouts;
		for( size_t ilayout = 0; ilayout < mLayouts.size(); ++ilayout )
		{
			const FaceLayout &layout = mLayouts[ ilayout ];
			if( ilayout == 0 &&
				( layout < FaceLayout() || FaceLayout() < layout ) )
			{
				// Note that this only applies if any layouts are stored at all.
				err << "Layout 0 must be the default layout" << endl;
				return false;
			}
			if( layout.side_range[ 0 ] >= layout.side_range[ 1 ] ||
				layout.side_range[ 1 ] >= layout.side_range[ 2 ] ||
				layout.side_range[ 2 ] >= layout.side_range[ 3 ] ||
				layout.side_range[ 3 ] >= layout.side_range[ 4 ] )
			{
				err << "Layout " << ilayout << " doesn't have at least one edge on each side" << endl;
				return false;
			}

			layouts.insert( layout );
		}
		if( layouts.size() != mLayouts.size() )
		{
			err << "Some of the face layouts were duplicates. Face layouts should be unique." << endl;
			return false;
		}
		if( mLayouts.size() > 1 && mIntervals.empty() )
		{
			err << "This model has multiple face layouts but doesn't specify edge intervals." << endl;
			return false;
		}
		// Loop through all the pieces
		for( size_t ipiece = 0; ipiece < mPieces.size(); ++ipiece )
		{
			const Piece2D &piece = mPieces[ ipiece ];
			if( piece.pt_index >= mPoints.size() )
			{
				err << "Piece " << ipiece << " has an OOB pt_index" << endl;
				return false;
			}
			int s_order = piece.st_order & 0xFFFF;
			int t_order = piece.st_order >> 16;
			if( piece.maybe_t_index == INVALID_INDEX )
			{
				// Explicit piece validity check
				size_t piece_size = s_order * t_order;
				if( piece.s_index + piece_size > mCoeffs.size() )
				{
					err << "Piece " << ipiece << " refers to OOB coefficients" << endl;
					return false;
				}
			}
			else
			{
				// Tensor-produce piece validity check
				if( piece.s_index + s_order > mCoeffs.size() )
				{
					err << "Piece " << ipiece << " in S (TP) refers to OOB coefficients" << endl;
					return false;
				}
				if( piece.maybe_t_index + t_order > mCoeffs.size() )
				{
					err << "Piece " << ipiece << " in T (TP) refers to OOB coefficients" << endl;
					return false;
				}
			}
		}
		// Verify that the edges and the intervals have matching sizes.
		if( !mIntervals.empty() && mEdges.size() != mIntervals.size() )
		{
			err << "The interval and the edge vectors must be the same size (unless intervals is empty)" << endl;
			return false;
		}
		// Loop through all the intervals.
		for( size_t iinterval = 0; iinterval < mIntervals.size(); ++iinterval )
		{
			if( mIntervals[ iinterval ] < 0.0 || !finite( mIntervals[ iinterval ] ) )
			{
				err << "Interval on edge " << iinterval << " has an illegal value (must be >= 0.0 and finite)" << endl;
				return false;
			}
		}
		// Loop through all the edges (table of element adjacency)
		for( size_t iedge = 0; iedge < mEdges.size(); ++iedge )
		{
			if( mEdges[ iedge ] != INVALID_INDEX && mEdges[ iedge ] >= mElems.size() )
			{
				err << "Edge " << iedge << " is adjacent to an OOB element" << endl;
				return false;
			}
		}
		// Loop through all the elements.
		for( size_t ielem = 0; ielem < mElems.size(); ++ielem )
		{
			// The diagnostic messages are a bit vague but will hopefully give enough of a clue
			// to find problems.
			const Elem &elem = mElems[ ielem ];
			if( elem.edge_end_index < last_edge_end )
			{
				err << "Elem " << ielem << " has an edge_end_index < last_edge_end" << endl;
				return false;
			}
			if( elem.edge_end_index > mEdges.size() )
			{
				err << "Elem " << ielem << " has an edge_end_index > mEdges.size()" << endl;
				return false;
			}
			if( elem.piece_end_index < last_piece_end )
			{
				err << "Elem " << ielem << " has a piece_end_index < last_piece_end" << endl;
				return false;
			}
			if( elem.piece_end_index > mPieces.size() )
			{
				err << "Elem " << ielem << " has a piece_end_index > mPieces.size()" << endl;
				return false;
			}
			if( elem.layout_index != 0 && elem.layout_index >= mLayouts.size() )
			{
				err << "Elem " << ielem << " has a layout_index >= mLayouts.size()" << endl;
				return false;
			}
			// Layout 0 is not required to be explicitly stored.
			if( elem.layout_index < mLayouts.size() )
			{
				const FaceLayout &layout = mLayouts[ elem.layout_index ];
				if( elem.edge_end_index - last_edge_end != layout.side_range[ 4 ] )
				{
					err << "Elem " << ielem << " has " << ( elem.edge_end_index - last_edge_end ) << " edges but its layout has " << ( layout.side_range[ 4 ] ) << " edges" << endl;
					return false;
				}
			}
			last_edge_end = elem.edge_end_index;
			last_piece_end = elem.piece_end_index;
		}
		if( last_edge_end != mEdges.size() || last_piece_end != mPieces.size() )
		{
			err << "The Elems do not refer to all the edges/pieces" << endl;
			return false;
		}
		return true;
	}

	// Needed to make the NothingStream.
	class NothingBuffer: public std::streambuf
	{
	public:
		int overflow( int c ) override { return c; }
	};

	// A quick-and-dirty "do-nothing" output stream
	class NothingStream : public std::ostream
	{
	public:
		NothingStream() : std::ostream( &mBuffer ) {}
	private:
		NothingBuffer mBuffer;
	};

	bool IGAData::isValid() const
	{
		NothingStream no_out;
		return isValid( no_out );
	}

	const FaceLayout &IGAData::layout( uint32_t layout_index ) const
	{
		const static FaceLayout s_default_layout;
		// Implements the requirement that if no layouts are stored at all, we should return the
		// default layout, and that layout 0 always be the default layout.
		if( layout_index == 0 )
			return s_default_layout;
		return mLayouts[ layout_index ];
	}

	uint32_t IGAData::layoutIndex( uint32_t elem_index ) const
	{
		return mElems[ elem_index ].layout_index;
	}

	uint32_t IGAData::pieceBegin( uint32_t elem_index ) const
	{
		if( elem_index == 0 )
			return 0u;
		else
			return mElems[ elem_index - 1 ].piece_end_index;
	}

	uint32_t IGAData::pieceCount() const
	{
		return static_cast< uint32_t >( mPieces.size() );
	}

	uint32_t IGAData::pieceEnd( uint32_t elem_index ) const
	{
		return mElems[ elem_index ].piece_end_index;
	}

	const double *IGAData::pieceExplicitCoeffs( uint32_t piece_index ) const
	{
		return &mCoeffs[ mPieces[ piece_index ].s_index ];
	}

	bool IGAData::pieceIsExplicit( uint32_t piece_index ) const
	{
		return mPieces[ piece_index ].maybe_t_index == INVALID_INDEX;
	}

	bool IGAData::pieceIsTensor( uint32_t piece_index ) const
	{
		return mPieces[ piece_index ].maybe_t_index != INVALID_INDEX;
	}

	const Point3d &IGAData::piecePoint( uint32_t piece_index ) const
	{
		return mPoints[ mPieces[ piece_index ].pt_index ];
	}

	uint32_t IGAData::piecePointIndex( uint32_t piece_index ) const
	{
		return mPieces[ piece_index ].pt_index;
	}

	const uint32_t IGAData::pieceSIndex( uint32_t piece_index ) const
	{
		return mPieces[ piece_index ].s_index;
	}

	const double *IGAData::pieceSCoeffs( uint32_t piece_index ) const
	{
		return &mCoeffs[ mPieces[ piece_index ].s_index ];
	}

	int IGAData::pieceSOrder( uint32_t piece_index ) const
	{
		return mPieces[ piece_index ].st_order & 0xFFFF;
	}

	const uint32_t IGAData::pieceTIndex( uint32_t piece_index ) const
	{
		return mPieces[ piece_index ].maybe_t_index;
	}

	const double *IGAData::pieceTCoeffs( uint32_t piece_index ) const
	{
		return &mCoeffs[ mPieces[ piece_index ].maybe_t_index ];
	}

	int IGAData::pieceTOrder( uint32_t piece_index ) const
	{
		return mPieces[ piece_index ].st_order >> 16;
	}

	uint32_t IGAData::pointCount() const
	{
		return static_cast< uint32_t >( mPoints.size() );
	}

	uint32_t IGAData::sideBegin( uint32_t elem_index, int side ) const
	{
		const FaceLayout &elem_layout = layout( mElems[ elem_index ].layout_index );
		return elem_layout.side_range[ side ];
	}

	uint32_t IGAData::sideEnd( uint32_t elem_index, int side ) const
	{
		const FaceLayout &elem_layout = layout( mElems[ elem_index ].layout_index );
		return elem_layout.side_range[ side + 1 ];
	}

}
