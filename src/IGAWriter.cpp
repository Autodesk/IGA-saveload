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

#include "iga/IGAWriter.h"

#include "iga/IGACommon.h"
#include "iga/IGAData.h"

namespace iga_fileio
{
	bool IGAWriter::writeBlock( const char *block_type, const char *contents, size_t length, uint64_t id )
	{
		// Block header
		if( !writeData( "\nBLOCK:\n", 8 ) )
			return false;
		uint64_t tag = tagValue( block_type );
		// Block tag
		if( !writeData( reinterpret_cast< const char * >( &tag ), 8 ) )
			return false;
		// Block ID
		if( !writeData( reinterpret_cast< const char * >( &id ), 8 ) )
			return false;
		// size_t isn't always 64 bits but we must have a 64-bit value.
		uint64_t len64 = length;
		// Block prefix length
		if( !writeData( reinterpret_cast< const char * >( &len64 ), 8 ) )
			return false;
		// Block contents
		if( !writeData( contents, length ) )
			return false;
		// Block postfix length
		if( !writeData( reinterpret_cast< const char * >( &len64 ), 8 ) )
			return false;
		return true;
	}
	
	bool IGAWriter::writeIGAFile( const IGAData &geometry )
	{
		#define WRITE_BLOCK( NAME, GETTER, TYPE ) \
		if( !writeBlock( NAME, reinterpret_cast< const char * >( geometry. GETTER ().data() ), geometry. GETTER ().size() * sizeof( TYPE ) ) ) return false

		// Write TSS header
		if( !writeData( "#TSS0001", 8 ) )
			return false;

		// Write IGAFILE block
		if( !writeBlock( "IGAFILE", "", 0 ) )
			return false;

		// Write SRFTYPE block
		WRITE_BLOCK( "SRFTYPE", surfaceType, char );

		// Write VECDICT block
		WRITE_BLOCK( "VECDICT", coeffs, double );

		// Write PT3DW block
		WRITE_BLOCK( "PT3DW", points, Point3d );

		// Write 2DPIECE block
		WRITE_BLOCK( "2DPIECE", pieces, Piece2D );

		// Write LAYOUT block
		WRITE_BLOCK( "LAYOUT", layouts, FaceLayout );

		// Write EDGES block
		WRITE_BLOCK( "EDGES", edges, uint32_t );

		// Write KNOTINT block if it's not empty
		if( geometry.intervals().size() > 0 )
		{
			WRITE_BLOCK( "KNOTINT", intervals, double );
		}

		// Write SHAPE block
		WRITE_BLOCK( "SHAPE", elems, Elem );

		writeFinished();

		return true;
	}
}