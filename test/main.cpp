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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "iga/IGAFileIO.h"

using std::cerr;
using std::cout;
using std::endl;

bool verbose = false;

// A class to let you read IGA files using standard istreams. You can load
// IGA data from any type for which you can implement this reader interface.
class IGAStreamReader: public iga_fileio::IGAReader
{
public:
	IGAStreamReader( std::istream &stream );

	bool readData( char *destination, size_t length ) override;

	void readFinished() override
	{
		if( verbose )
			std::cout << "readFinished() event received." << endl;
	}
private:
	std::istream *mStream = nullptr;
};

IGAStreamReader::IGAStreamReader( std::istream &stream )
	: mStream( &stream )
{
}

bool IGAStreamReader::readData( char *destination, size_t length )
{
	mStream->read( destination, length );
	return mStream->gcount() == length;
}

// A class to let you write IGA files using standard ostreams. You can write
// IGA data to any type for which you can implement this writer interface.
class IGAStreamWriter: public iga_fileio::IGAWriter
{
public:
	IGAStreamWriter( std::ostream &stream );

	bool writeData( const char *data_block, size_t length ) override;

	void writeFinished() override
	{
		if( verbose )
			std::cout << "writeFinished() event received." << endl;
	}
private:
	std::ostream *mStream = nullptr;
};

IGAStreamWriter::IGAStreamWriter( std::ostream &stream )
	: mStream( &stream )
{
}

bool IGAStreamWriter::writeData( const char *data_block, size_t length )
{
	mStream->write( data_block, length );
	return mStream->good();
}

void printVerboseIGA( const iga_fileio::IGAData &iga, std::ostream &o )
{
	// This output was written using only ostream to avoid a dependency on std::fmt,
	// which is not yet widely available.
	o << "Surface type is " << iga.surfaceType() << endl;

	for( unsigned ielem = 0; ielem < iga.elemCount(); ++ielem )
	{
		o << "Elem " << ielem << " has " << iga.elemEdgeCount( ielem ) << " edges (";
		for( int iside = 0; iside < 4; ++iside )
		{
			if( iside != 0 )
				o << ", ";
			o << iga.elemEdgesOnSide( ielem, iside );
		}
		o << " and covers pieces " << iga.pieceBegin( ielem ) << ".." << iga.pieceEnd( ielem ) << "." << endl;

		for( auto iedge = iga.edgeBegin( ielem ); iedge < iga.edgeEnd( ielem ); ++iedge )
		{
			if( iga.edgeOther( iedge ) == iga_fileio::INVALID_INDEX )
				o << "  Edge " << iedge << " is on the boundary and has interval " << iga.edgeInterval( iedge ) << endl;
			else
				o << "  Edge " << iedge << " is adjacent to Elem " << iga.edgeOther( iedge ) << " and has interval " << iga.edgeInterval( iedge ) << endl;
		}

		for( auto ipiece = iga.pieceBegin( ielem ); ipiece != iga.pieceEnd( ielem ); ++ipiece )
		{
			int s_order = iga.pieceSOrder( ipiece ), t_order = iga.pieceTOrder( ipiece );
			bool is_tp = iga.pieceIsTensor( ipiece );
			const auto &pt = iga.piecePoint( ipiece );
			o << "  Piece " << ipiece << " is " << ( is_tp ? "tensor product" : "explicit" ) << ", order " << s_order << "x" << t_order << ", geometry (" << pt.x << "," << pt.y << "," << pt.z << "/" << pt.w << ") and has coeffs:" << endl;
			if( is_tp )
			{
				const double *s_coeffs = iga.pieceSCoeffs( ipiece ),
					*t_coeffs = iga.pieceTCoeffs( ipiece );
				o << "    S:";
				for( int s = 0; s < iga.pieceSOrder( ipiece ); ++s )
					o << " " << s_coeffs[ s ];
				o << endl;
				o << "    T:";
				for( int t = 0; t < iga.pieceTOrder( ipiece ); ++t )
					o << " " << t_coeffs[ t ];
				o << endl;
			}
			else
			{
				const double *coeffs = iga.pieceExplicitCoeffs( ipiece );
				for( int t = iga.pieceTOrder( ipiece ); t--; )
				{
					o << "    ";
					for( int s = 0; s < iga.pieceSOrder( ipiece ); ++s )
					{
						o << coeffs[ s + t * s_order ] << " ";
					}
					o << endl;
				}
			}
		}
	}
}

int main( int argc, char **argv )
{
	if( argc < 2 )
	{
		std::cerr << "Usage: " << argv[ 0 ] << " filename.iga [--verbose]" << endl;
		return 1;
	}

	if( argc > 2 && std::string( argv[ 2 ] ) == "--verbose" )
	{
		verbose = true;
	}

	// Create a file stream using the passed filename.
	std::ifstream in_file( argv[ 1 ], std::ios::in | std::ios::binary );
	if( !in_file.good() )
	{
		std::cerr << "Failed to open that file." << endl;
		std::cerr << "Usage: " << argv[ 0 ] << " filename.iga" << endl;
		return 2;
	}

	// Use that file stream and our reader class to load some IGA data.
	IGAStreamReader reader( in_file );
	iga_fileio::IGAData iga_data;
	if( !reader.readIGAFile( iga_data ) )
	{
		std::cerr << "Failed to load valid data from that file." << endl;
		return 3;
	}

	if( !iga_data.isValid( cerr ) )
	{
		cerr << " ===== The IGA file is not valid." << endl;
		return 4;
	}
	else
		cout << "Loaded the IGA file; it contains " << iga_data.elemCount() << " elements." << endl;
	// Print out the data that we loaded.
	if( verbose )
		printVerboseIGA( iga_data, cout );

	// A simple demonstration of how to write IGA data to a file. For simplicity, we'll
	// just re-output the same data we just read in. Note that if the input IGA file had
	// any unrecognized blocks, this will "lose" that data.
	std::stringstream out_stream;
	IGAStreamWriter writer( out_stream );
	bool ok = writer.writeIGAFile( iga_data );
	if( !ok )
	{
		cerr << "Writing the IGA file failed." << endl;
		return 5;
	}
	cout << "Writing the IGA file to a buffer produced " << out_stream.str().size() << " bytes." << endl;

	return 0;
}