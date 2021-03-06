cmake_minimum_required (VERSION 2.8)

project (TrainGame)

set (${PROJECT_NAME}_MAJOR_VERSION 0)
set (${PROJECT_NAME}_MINOR_VERSION 3)
set (${PROJECT_NAME}_PATCH_LEVEL 0-dev) 

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Make sure optimisation is always enabled
if (UNIX)
  if (CMAKE_COMPILER_IS_GNUCC)
    set (CMAKE_CXX_FLAGS "-g -Wall -ffast-math -O2 -std=c++0x")
    # "-Wconversion -Werror"
    if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2 -mfpmath=sse")
    endif ()
  else (CMAKE_COMPILER_IS_GNUCC)
    # Just accept default options
    # TODO: add support for Clang here
    set (CMAKE_BUILD_TYPE RelWithDebInfo)
  endif ()
else (UNIX)
  # Just accept default options
  set (CMAKE_BUILD_TYPE RelWithDebInfo)
endif (UNIX)

# Find all the source files
file (GLOB_RECURSE folder_source 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
source_group (${PROJECT_NAME} FILES ${folder_source})

find_package (SDL REQUIRED)
find_package (SDL_image REQUIRED)
find_package (OpenGL REQUIRED)
find_package (GLEW REQUIRED)
find_package (Boost 1.37 REQUIRED 
  COMPONENTS filesystem signals program_options system) 
find_package (Freetype REQUIRED)

if (NOT WIN32)
  include (FindPkgConfig)
endif (NOT WIN32)

find_package (Xerces REQUIRED)

include_directories (${CMAKE_SOURCE_DIR}/include)
include_directories (${CMAKE_SOURCE_DIR}/src)
include_directories (include ${SDL_INCLUDE_DIR})
include_directories (include ${SDL_IMAGE_INCLUDE_DIR})
include_directories (include ${OPENGL_INCLUDE_DIR})
include_directories (include ${GLEW_INCLUDE_DIR})
include_directories (include ${XERCES_INCLUDE_DIRS})
include_directories (include ${FREETYPE_INCLUDE_DIRS})
include_directories (include ${CMAKE_CURRENT_BINARY_DIR})

# WIN32 makes a non-console application on Windows
add_executable (${PROJECT_NAME} WIN32 ${folder_source})

target_link_libraries (${PROJECT_NAME} ${SDL_LIBRARY} ${SDLIMAGE_LIBRARY}
  ${OPENGL_LIBRARY} ${OpenGL_GLU_LIBRARY} ${XERCES_LIBRARIES} ${Boost_LIBRARIES}
  ${FREETYPE_LIBRARIES} ${GLEW_LIBRARY})

# Test tool
add_executable (MathsTest EXCLUDE_FROM_ALL tools/MathsTest.cpp)

# Profiling
if (PROFILE)
  set_target_properties (${PROJECT_NAME} PROPERTIES LINK_FLAGS -pg)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif (PROFILE)

# GCC STL profiling
if (STL_PROFILE)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_PROFILE")
endif (STL_PROFILE)

set (REL "${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}")
set (PKGNAME "${PROJECT_NAME}-${REL}")

# Installation
install (PROGRAMS bin/${PROJECT_NAME} 
  DESTINATION bin)
install (DIRECTORY data maps buildings schemas waggons engines
  DESTINATION .)
install (FILES COPYING.txt README.txt COPYRIGHT.vera 
  DESTINATION .)

# Packaging
include (InstallRequiredSystemLibraries)

set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Train Game!")
set (CPACK_PACKAGE_VENDOR "doof.me.uk")
set (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.txt")
set (CPACK_PACKAGE_VERSION_MAJOR "${${PROJECT_NAME}_MAJOR_VERSION}")
set (CPACK_PACKAGE_VERSION_MINOR "${${PROJECT_NAME}_MINOR_VERSION}")
set (CPACK_PACKAGE_VERSION_PATCH "${${PROJECT_NAME}_PATCH_LEVEL}")
set (CPACK_PACKAGE_INSTALL_DIRECTORY "${PKGNAME}")
if (WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  set (CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
  set (CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\MyExecutable.exe")
  set (CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} My Famous Project")
  set (CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.my-project-home-page.org")
  set (CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.my-personal-home-page.com")
  set (CPACK_NSIS_CONTACT "nick@nickg.me.uk")
  set (CPACK_NSIS_MODIFY_PATH ON)
else (WIN32 AND NOT UNIX)
  set (CPACK_STRIP_FILES "bin/${PROJECT_NAME}")
  set (CPACK_SOURCE_STRIP_FILES "")
endif (WIN32 AND NOT UNIX)
set (CPACK_PACKAGE_EXECUTABLES "bin/${PROJECT_NAME}")

# Generators to use
set (CPACK_GENERATOR "STGZ;TGZ")

include (CPack)

configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
