set(blockchain_db_sources
  lmdb/db_lmdb.c
  )

# if (BERKELEY_DB)
#   set(blockchain_db_sources
#   ${blockchain_db_sources}
#   berkeleydb/db_bdb.cpp
#   )
# endif()

set(blockchain_db_headers)

set(blockchain_db_private_headers
  blockchain_db.h
  lmdb/db_lmdb.h
  )

# if (BERKELEY_DB)
#   set(blockchain_db_private_headers
#   ${blockchain_db_private_headers}
#   berkeleydb/db_bdb.h
#   )
# endif()

monero_private_headers(blockchain_db
  ${crypto_private_headers})
monero_add_library(blockchain_db
  ${blockchain_db_sources}
  ${blockchain_db_headers}
  ${blockchain_db_private_headers})

target_link_libraries(blockchain_db
	PUBLIC
	common
    ${LMDB_LIBRARY}
	)

