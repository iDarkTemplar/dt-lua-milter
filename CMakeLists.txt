project(DT-Lua-Milter)

cmake_minimum_required( VERSION 2.8.8 )

set(DLM_VERSION_MAJOR 0)
set(DLM_VERSION_MINOR 1)
set(DLM_VERSION_PATCH 0)
set(DLM_VERSION "${DLM_VERSION_MAJOR}.${DLM_VERSION_MINOR}.${DLM_VERSION_PATCH}")

set(CPACK_PACKAGE_VERSION_MAJOR ${DLM_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DLM_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DLM_VERSION_PATCH})
set(CPACK_GENERATOR "TBZ2")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES
	".git;build;CMakeLists.txt.user;CMakeLists.txt.user.*;TODO")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PCC_VERSION}")
# MUST be after CPACK_* variables.
include(CPack)

# installation directory configuration
set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
set(EXEC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Installation prefix for executables and object code libraries" FORCE)
set(BIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/bin CACHE PATH "Installation prefix for user executables" FORCE)
set(SBIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/sbin CACHE PATH "Installation prefix for user executables" FORCE)
set(LIB_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX} CACHE PATH  "Installation prefix for object code libraries" FORCE)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include CACHE PATH "Installation prefix for C header files" FORCE)
set(DATA_PREFIX "${CMAKE_INSTALL_PREFIX}/share" CACHE PATH "Prefix prepended to data install directories" FORCE)

# hack to use my milter search module
SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_extra )

#FIND_PACKAGE( Boost COMPONENTS regex REQUIRED )
#INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

if (UNIX)
	set( CMAKE_THREAD_PREFER_PTHREAD 1 )
endif (UNIX)
find_package (Threads REQUIRED)

find_package (Milter REQUIRED)
find_package (Lua REQUIRED)

# don't USE -O3 with GCC, it causes less precise calculations
if (CMAKE_COMPILER_IS_GNUCC)
	set (CMAKE_C_FLAGS_RELEASE "-O2 -pipe -march=native -Wall -Wextra")
	set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
endif (CMAKE_COMPILER_IS_GNUCC)

# clang compiler options
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set (CLANG_WARNINGS "-Weverything -Wno-missing-prototypes -Wno-padded -Wno-sign-conversion -Wno-conversion -Wno-disabled-macro-expansion -Wno-cast-align" )
	set (CMAKE_C_FLAGS_RELEASE "-O3 -pipe ${CLANG_WARNINGS}")
	set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

add_definitions(-D_FILE_OFFSET_BITS=64)

set ( SOURCES dt-lua-milter.c )
set ( HEADERS )

add_executable( dt-lua-milter ${SOURCES} ${HEADERS})
target_link_libraries( dt-lua-milter ${MILTER_LIBRARIES} ${LUA_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )

# installation config
install(TARGETS dt-lua-milter RUNTIME DESTINATION ${SBIN_INSTALL_DIR})
