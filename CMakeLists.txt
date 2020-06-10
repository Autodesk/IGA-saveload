# Set the CMake version
cmake_minimum_required( VERSION 3.9 )

# Write a .gitignore file so that git will ignore CMake-generated files.
file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/.gitignore "# Ignore all files in this directory\n*\n" )

# The project name
project( IGA-saveload )

# Organize the files into folders / groups
set( IGA_CPP_FILES
	src/IGACommon.cpp
	src/IGACreator.cpp
	src/IGAData.cpp
	src/IGAReader.cpp
	src/IGAWriter.cpp
)
set( IGA_H_FILES
	include/iga/IGACommon.h
	include/iga/IGACreator.h
	include/iga/IGAData.h
	include/iga/IGAFileIO.h
	include/iga/IGAReader.h
	include/iga/IGAWriter.h
)
source_group( "Source" FILES ${IGA_CPP_FILES} test/main.cpp )
source_group( "Headers" FILES ${IGA_H_FILES} )

# A simple executable that uses our source files.
add_executable( IGA-saveload ${IGA_CPP_FILES} ${IGA_H_FILES} test/main.cpp )

# Include directories
target_include_directories( IGA-saveload PRIVATE
	include/
)
