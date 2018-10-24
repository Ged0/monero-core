set(crypto_sources
	hash.c
	)

set(crypto_headers)

set(crypto_private_headers
	crypto.h
	hash-ops.h
  	hash.h
	)


monero_private_headers(cncrypto
  ${crypto_private_headers})
monero_add_library(cncrypto
  ${crypto_sources}
  ${crypto_headers}
  ${crypto_private_headers})
