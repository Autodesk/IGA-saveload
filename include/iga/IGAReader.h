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

#ifndef IGA_READER_H_
#define IGA_READER_H_

#include <vector>

namespace iga_fileio
{
	class IGAData;

	/// A pure virtual base class for reading IGA data from a stream or file.
	class IGAReader
	{
	public:
		/// Read a block of data from the stream. It is guaranteed that the
		/// destination will already have enough bytes allocated to contain
		/// 'length' entries, but no more. In general, destination is not
		/// a string, so you shouldn't do anything additional like adding a
		/// null-terminator
		///
		/// If your implementation returns true, then the data will be treated
		/// as having been read successfully. Note that your implementation must
		/// return failure if the requested amount of data was not read, as this
		/// is the primary way we can detect whether we have reached the end of
		/// the data to load.
		///
		/// If you are using an fstream, this implementation should work:
		///
		/// { return !read( static_cast< char * >( destination, length ).bad(); }
		virtual bool readData( char *destination, size_t length ) = 0;

		/// This will be called after the final file read is finished to
		/// allow you to do any cleanup you need. Possibly useful for closing
		/// the file or similar.
		virtual void readFinished() {}

		/// The core read function. Generates a series of readData calls
		/// and returns true if reading the file succeeds.
		bool readIGAFile( IGAData &geometry );

	private:
		template< typename T >
		bool readBlock( std::vector< T > &dst, size_t len );
	};
}

#endif