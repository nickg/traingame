cmake_minimum_required (VERSION 2.6)

project (TrainGame)

set (${PROJECT_NAME}_MAJOR_VERSION 0)
set (${PROJECT_NAME}_MINOR_VERSION 1)
set (${PROJECT_NAME}_PATCH_LEVEL 0) 

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# Make sure optimisation is always enabled
set (CMAKE_BUILD_TYPE RelWithDebInfo)

# Find all the source files
file (GLOB folder_source ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
source_group (${PROJECT_NAME} FILES ${folder_source})

include_directories (${CMAKE_SOURCE_DIR}/include)

if (NOT WIN32) # Unix
    add_definitions ("-Wall")
endif (NOT WIN32)

# WIN32 makes a non-console application on Windows
add_executable (${PROJECT_NAME} WIN32 ${folder_source})
