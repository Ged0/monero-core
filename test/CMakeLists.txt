set(test_sources
	lmdb_test.c
	)

set(test_headers)

add_executable(lmdb_tests
	${test_headers}
	${test_sources}
	)

target_link_libraries(lmdb_tests
	PRIVATE
	common
	blockchain_db
    ${LMDB_LIBRARY}
	${GLIB_LDFLAGS})