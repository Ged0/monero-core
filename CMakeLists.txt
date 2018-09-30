cmake_minimum_required (VERSION 3.3.1)
project (monero_core C)

set(EXECUTABLE_NAME monero_core)

################### find dependencies #######################
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(LMDB REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
message("glib include: ${GLIB_INCLUDE_DIRS}")
message("glib libraries: ${GLIB_LDFLAGS}")

message("CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
# add_subdirectory(src)
# add_subdirectory(test)

set(SOURCE_FILES 
    test/lmdb_test.c)
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

if(LMDB_FOUND)
    message("lmdb include: ${LMDB_INCLUDE_DIR}")
    message("lmdb libraries: ${LMDB_LIBRARIES}")
    INCLUDE_DIRECTORIES(${LMDB_INCLUDE_DIR})
    target_link_libraries(${EXECUTABLE_NAME} ${LMDB_LIBRARIES})
endif(LMDB_FOUND)

INCLUDE_DIRECTORIES(${GLIB_INCLUDE_DIRS})
target_link_libraries(${EXECUTABLE_NAME} ${GLIB_LDFLAGS})
