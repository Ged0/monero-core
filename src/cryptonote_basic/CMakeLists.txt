set(cryptonote_basic_sources
	difficulty.c
	)

set(cryptonote_basic_headers)

set(cryptonote_basic_private_headers
  cryptonote_basic.h
  difficulty.h)

monero_private_headers(cryptonote_basic
  ${cryptonote_basic_private_headers})
monero_add_library(cryptonote_basic
  ${cryptonote_basic_sources}
  ${cryptonote_basic_headers}
  ${cryptonote_basic_private_headers})