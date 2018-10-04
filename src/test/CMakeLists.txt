set(blockchain_db_test_sources
	lmdb_test.c)

monero_add_executable(blockchain_db_test
  ${blockchain_db_test_sources})

target_link_libraries(blockchain_db_test 
	PRIVATE 
	common
	blockchain_db
	${GLIB_LDFLAGS}
	${GLIB_LDFLAGS})