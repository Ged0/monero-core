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
include_directories(src)
# include_directories(src/blockchain_db)
set(SOURCE_FILES 
    test/lmdb_test.c
    src/blockchain_db/blockchain_db.h
    src/blockchain_db/lmdb/db_lmdb.h
    src/blockchain_db/lmdb/db_lmdb.c
    )
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

if(LMDB_FOUND)
    message("lmdb include: ${LMDB_INCLUDE_DIR}")
    message("lmdb libraries: ${LMDB_LIBRARIES}")
    include_directories(${LMDB_INCLUDE_DIR})
    target_link_libraries(${EXECUTABLE_NAME} ${LMDB_LIBRARIES})
endif(LMDB_FOUND)

include_directories(${GLIB_INCLUDE_DIRS})
target_link_libraries(${EXECUTABLE_NAME} ${GLIB_LDFLAGS})
