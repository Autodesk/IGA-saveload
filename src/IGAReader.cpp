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

#include "iga/IGAReader.h"

#include "iga/IGACommon.h"
#include "iga/IGAData.h"

namespace iga_fileio
{
	template< typename T >
	bool IGAReader::readBlock( std::vector< T > &dst, size_t len )
	{
		// Sizes must exactly fit the struct size
		if( len % sizeof( T ) != 0 )
			return false;
		size_t n = len / sizeof( T );

		if( len != 0 )
		{
			dst.clear();
			if( len >= IGA_MAX_ALLOC )
				return false;
			dst.resize( n );
			char *target_ptr = reinterpret_cast< char * >( dst.data() );
			if( !readData( target_ptr, len ) )
				return false;
		}
		uint64_t final_len = ~0ull;
		if( !readData( reinterpret_cast< char * >( &final_len ), 8 ) )
			return false;
		if( final_len != len )
			return false;
		return true;
	}

	bool IGAReader::readIGAFile( IGAData &geometry )
	{
		geometry.clear();

		// Read and check TSS header
		{
			// Showing up as a bounds-check warning; you can check the bounds yourself on the call
			// to readData a few lines down.
			// Flawfinder: ignore
			char buf[ 9 ] = "--------";
			if( !readData( buf, 8 ) ) return false;
			buf[ 8 ] = '\0';
			if( strcmp( buf, "#TSS0001" ) != 0 ) return false;
		}

		// Our interface doesn't have a 'seek' operation, only a 'read' operation, so
		// we still have to read the blocks that are not recognized. For those blocks,
		// we'll use this buffer as the read target.
		std::vector< char > unused_block;

		// Read first block.
		{
			BlockHeader block_header;
			if( !readData( reinterpret_cast< char * >( &block_header ), sizeof( BlockHeader ) ) ) return false;
			if( tagValue( block_header.block_tag ) != tagValue( "\nBLOCK:\n" ) ) return false;
			if( block_header.tag != tagValue( "IGAFILE" ) ) return false;
			if( !readBlock( unused_block, block_header.block_len ) ) return false;
		}

		// Read blocks in a loop until reading a block header fails.
		bool block_read_okay = false;
		do
		{
			// Read the next block's header.
			BlockHeader block_header;
			block_read_okay = readData( reinterpret_cast< char * >( &block_header ), sizeof( BlockHeader ) );
			// Failure to read the block header is how we detect end of file, so this is
			// an acceptable failure.
			if( !block_read_okay )
				break;
			if( tagValue( block_header.block_tag ) != tagValue( "\nBLOCK:\n" ) ) return false;

			// Check block type against known block types. Silently ignore unknown block
			// types, to enable forward compatibility.
			if( block_header.tag == tagValue( "SRFTYPE" ) )
			{
				geometry.clear();
				// You could build a method for reading strings directly and save a copy.
				std::vector< char > srf_type;
				if( !readBlock( srf_type, block_header.block_len ) ) return false;
				geometry.mSrfType.assign( srf_type.begin(), srf_type.end() );
			}
			else if( block_header.tag == tagValue( "VECDICT" ) )
			{
				if( !readBlock( geometry.mCoeffs, block_header.block_len ) ) return false;
			}
			else if( block_header.tag == tagValue( "PT3DW" ) )
			{
				if( !readBlock( geometry.mPoints, block_header.block_len ) ) return false;
			}
			else if( block_header.tag == tagValue( "2DPIECE" ) )
			{
				if( !readBlock( geometry.mPieces, block_header.block_len ) ) return false;
			}
			else if( block_header.tag == tagValue( "LAYOUT" ) )
			{
				if( !readBlock( geometry.mLayouts, block_header.block_len ) ) return false;
			}
			else if( block_header.tag == tagValue( "EDGES" ) )
			{
				if( !readBlock( geometry.mEdges, block_header.block_len ) ) return false;
			}
			else if( block_header.tag == tagValue( "KNOTINT" ) )
			{
				if( !readBlock( geometry.mIntervals, block_header.block_len ) ) return false;
			}
			else if( block_header.tag == tagValue( "SHAPE" ) )
			{
				if( !readBlock( geometry.mElems, block_header.block_len ) ) return false;
			}
			else
			{
				if( !readBlock( unused_block, block_header.block_len ) ) return false;
			}
		} while( block_read_okay );

		readFinished();
		return true;
	}

}