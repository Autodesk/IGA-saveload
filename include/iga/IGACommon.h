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

#ifndef IGA_COMMON_H_
#define IGA_COMMON_H_

#include <cstdint>
#include <vector>

namespace iga_fileio
{
	using std::uint32_t;
	using std::uint64_t;
	const uint32_t INVALID_INDEX = 0xFFFFFFFF;
	// This is a 'sanity' value for buffer allocation. If an IGA file tries to tell the reader
	// to load a buffer which would require more than this many bytes to store, it will instead
	// cause the read to fail.
	//
	// If this value is too restrictive for the file sizes you need to read, you may change it
	// to a value which is more useful for your application domain. It is strongly recommended
	// that the value you choose be something that your application can actually read.
	#ifndef IGA_MAX_ALLOC
	#define IGA_MAX_ALLOC 256000000
	#endif

	using CoeffVector = std::vector< double >;

	#ifdef _MSC_VER
	/// Cross-Platform macro to allow the use of finite() on Windows. Note that the
	/// specification differs between Posix and Visual Studio: on Windows, NAN is finite,
	/// but on Linux it is infinite. I emulate the Posix behavior here.
	template< class T >
	inline bool finite( T x ) { return ( _finite( ( double ) x ) && !_isnan( ( double ) x ) ); }
	#else
	template< class T >
	inline bool finite( T x ) { return std::isfinite( x ) != 0; }
	#endif

	/// Each block in the container file has a header with this structure.
	struct BlockHeader
	{
		// This shows up as a bounds-check warning. See the way we use this
		// array in IGAReader.cpp.
		// Flawfinder: ignore
		char block_tag[ 8 ] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		uint64_t tag = 0;
		uint64_t id = 0;
		uint64_t block_len = 0;
	};

	/// The tags in IGA/TSS files are 64-bit integers, but they are built
	/// from mnemonic strings. This converts from the mnemonic string format
	/// to the 64-bit integer format.
	///
	/// The tag_text should be a maximum of 8 characters, and should be a
	/// null-terminated string. Any characters after the eighth are ignored,
	/// and do not affect the returned value. If you include less than 8
	/// characters, then the tag is padded with "\n" in the 8th character,
	/// and hyphens '-' in all preceding blank characters. Note that the
	/// blank tag is the constant value 0uLL, and I don't provide any
	/// method for this function to return that.
	uint64_t tagValue( const char *tag_text );
}

#endif