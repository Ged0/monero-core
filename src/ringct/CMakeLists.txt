set(ringct_basic_sources
  rctTypes.c
  )

set(ringct_basic_private_headers
  rctTypes.h)

monero_private_headers(ringct_basic
  ${crypto_private_headers})
monero_add_library(ringct_basic
  ${ringct_basic_sources}
  ${ringct_basic_private_headers})

set(ringct_sources
  rctSigs.c
)

set(ringct_headers)

set(ringct_private_headers
  rctSigs.h
)

monero_private_headers(ringct
  ${crypto_private_headers})
monero_add_library(ringct
  ${ringct_sources}
  ${ringct_headers}
  ${ringct_private_headers})
target_link_libraries(ringct)