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

#ifndef IGA_WRITER_H_
#define IGA_WRITER_H_

#include <cstdint>

namespace iga_fileio
{
	class IGAData;

	/// A pure virtual base class for writing to a stream/file.
	class IGAWriter
	{
	public:
		/// Write a block of data. The data will be passed as a pointer to
		/// an array of characters with 'length' elements. The array is not
		/// a string, and may contain any character. The implementation should
		/// return true if the write was successful and false if it was not.
		virtual bool writeData( const char *data_block, size_t length ) = 0;

		/// Used to write a full block to the output file. If you don't
		/// provide an override for this function in your writer then it will
		/// be implemented in terms of writeData. If you do provide an
		/// override, then you will need to write the blocks in whatever way
		/// you see fit. Note that writeData is still called when writing
		/// the TSS header ("#TSS0001"), but is otherwise only used in the
		/// default implementation of writeBlock.
		///
		/// The default implementation will call tagValue() on the passed
		/// block_type, so you can use the short-form names for blocks safely.
		/// If you write your own, be aware that the name may be in short-form.
		///
		/// You should return true if the write operation succeeds, and false
		/// if it doesn't.
		virtual bool writeBlock( const char *block_type, const char *contents, size_t length, uint64_t id = 0 );

		/// This will be called after the final file write is finished to
		/// allow you to do any cleanup you need. You can use this to close
		/// the file, write auxiliary data, or provide an index block.
		virtual void writeFinished() {}

		/// The core writer function. Generates a series of writeBlock calls
		/// and returns true if they succeed.
		bool writeIGAFile( const IGAData &geometry );
	};
}

#endif