cmake_minimum_required (VERSION 3.3.1)
project (monero_core C)


################### find dependencies #######################
# set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_SOURCE_DIR}/cmake")


include(CheckCCompilerFlag)
include(CheckLinkerFlag)
include(CheckLibraryExists)
include(CheckFunctionExists)

# Check whether we're on a 32-bit or 64-bit system
if(CMAKE_SIZEOF_VOID_P EQUAL "8")
  set(DEFAULT_BUILD_64 ON)
else()
  set(DEFAULT_BUILD_64 OFF)
endif()
option(BUILD_64 "Build for 64-bit? 'OFF' builds for 32-bit." ${DEFAULT_BUILD_64})

if(BUILD_64)
  set(ARCH_WIDTH "64")
else()
  set(ARCH_WIDTH "32")
endif()
message(STATUS "Building for a ${ARCH_WIDTH}-bit system")


# function (die msg)
#   if (NOT WIN32)
#     string(ASCII 27 Esc)
#     set(ColourReset "${Esc}[m")
#     set(BoldRed     "${Esc}[1;31m")
#   else ()
#     set(ColourReset "")
#     set(BoldRed     "")
#   endif ()

#   message(FATAL_ERROR "${BoldRed}${msg}${ColourReset}")
# endfunction ()

# function (add_c_flag_if_supported flag var)
#   string(REPLACE "-" "_" supported ${flag}_c)
#   check_c_compiler_flag(${flag} ${supported})
#   if(${${supported}})
#     set(${var} "${${var}} ${flag}" PARENT_SCOPE)
#   endif()
# endfunction()

# function (add_linker_flag_if_supported flag var)
#   string(REPLACE "-" "_" supported ${flag}_ld)
#   string(REPLACE "," "_" supported ${flag}_ld)
#   check_linker_flag(${flag} ${supported})
#   if(${${supported}})
#     set(${var} "${${var}} ${flag}" PARENT_SCOPE)
#   endif()
# endfunction()

# function (add_definition_if_function_found function var)
#   string(REPLACE "-" "_" supported ${function}_function)
#   check_function_exists(${function} ${supported})
#   if(${${supported}})
#     add_definitions("-D${var}")
#   endif()
# endfunction()

# function (add_definition_if_library_exists library function header var)
#   string(REPLACE "-" "_" supported ${function}_library)
#   check_library_exists(${library} ${function} ${header} ${supported})
#   if(${${supported}})
#     add_definitions("-D${var}")
#   endif()
# endfunction()

# if(NOT CMAKE_BUILD_TYPE)
#   set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
#   message(STATUS "Setting default build type: ${CMAKE_BUILD_TYPE}")
# endif()
# string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)

# list(INSERT CMAKE_MODULE_PATH 0
#   "${CMAKE_SOURCE_DIR}/cmake")

# set_property(GLOBAL PROPERTY USE_FOLDERS ON)



# find_package(LMDB REQUIRED)
# message("lmdb include: ${LMDB_INCLUDE_DIR}")
# message("lmdb libraries: ${LMDB_LIBRARIES}")

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
message("glib include: ${GLIB_INCLUDE_DIRS}")
message("glib libraries: ${GLIB_LDFLAGS}")

message("CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
# add_subdirectory(src)
# add_subdirectory(test)
# include_directories(${LMDB_INCLUDE_DIR})
# include_directories(${LMDB_INCLUDE_DIR})
include_directories(${GLIB_INCLUDE_DIRS})
# message("LMDB_INCLUDE: ${LMDB_INCLUDE_DIR}")
message("GLIB_INCLUDE_DIRS: ${GLIB_INCLUDE_DIRS}")

# find_package(ZLOG REQUIRED)
# message("ZLOG_INCLUDE: ${ZLOG_INCLUDE}")
# message("ZLOG_LIBRARIES: ${ZLOG_LIBRARIES}")

add_subdirectory(external)
include_directories(${LMDB_INCLUDE})

include_directories(src)
add_subdirectory(src)