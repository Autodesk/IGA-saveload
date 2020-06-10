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

#include "iga/IGACommon.h"

namespace iga_fileio
{
	uint64_t tagValue( const char *tag_text )
	{
		// This is the value we're constructing.
		uint64_t result = 0;
		// Iterate through the tag_text
		const char *c = tag_text;
		// Iterate through the output (result)
		char *t;
		unsigned char_count = 0;
		t = reinterpret_cast< char * >( &result );
		while( *c && char_count < 8 )
		{
			// Copy char-by-char from tag_text.
			*( t++ ) = *( c++ );
			++char_count;
		}
		// Pad any remaining blanks.
		while( char_count < 8 )
		{
			// Last character
			if( char_count == 7 )
				*t = '\n';
			// Pad with hyphens
			else
				*( t++ ) = '-';
			++char_count;
		}
		return result;
	}
}
